// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT
//
// 개발·자동화용 콘솔 커맨드. 우클릭 메뉴와 같은 액션을 옵션 다이얼로그 없이 실행한다.
// 설정은 항상 기본값에서 시작해 인자로만 바뀐다 (다이얼로그에 저장된 옵션은 사용하지 않음).
//
//   MIS.AnalyzeSelected       [key=value ...]   드라이 런, 아일랜드/파트 개수를 로그로 출력
//   MIS.SplitSelected         [key=value ...]   콘텐츠 브라우저에서 선택한 각 스태틱 메시 옆에 파트 애셋 생성
//   MIS.SplitSelectedActors   [key=value ...]   레벨에서 선택한 스태틱 메시 액터를 분할 후 파트 액터로 치환
//
// 키:
//   mode=connectivity|proximity|material   (기본값 proximity)
//   dist=<cm>          절대 병합 거리
//   pct=<percent>      바운즈 대각선 대비 상대 병합 거리
//   weld=<cm>          용접 허용 오차, 0이면 용접 비활성화
//   pivot=original|center|bottom
//   matboundary=0|1    머티리얼 경계를 넘어 병합하지 않음
//   suffix=<str>       파트 이름 접미사 (기본 _Part_)
//   start=<n>          첫 인덱스 (기본 0)
//   digits=<n>         인덱스 자릿수 (기본 2)
//   folder=<name>      원본 폴더 아래 하위 폴더에 저장
//   replace=0|1        (SplitSelected) 레벨에서 원본을 쓰는 액터를 모두 파트로 치환
//   keep=0|1           치환 시 원본 액터를 삭제하지 않고 남김
//
// 예: MIS.AnalyzeSelected mode=proximity dist=1.5 matboundary=1

#include "MISEditorActions.h"
#include "MISMeshSplitter.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"

namespace MISConsole
{
	void ParseSettings(const TArray<FString>& Args, FMISSplitSettings& Out, bool& bOutReplace, FMISReplaceSettings& OutReplace)
	{
		for (const FString& Arg : Args)
		{
			FString Key;
			FString Value;
			if (!Arg.Split(TEXT("="), &Key, &Value))
			{
				UE_LOG(LogMISSplitter, Warning, TEXT("Ignoring argument '%s' (expected key=value)"), *Arg);
				continue;
			}

			if (Key.Equals(TEXT("mode"), ESearchCase::IgnoreCase))
			{
				if (Value.StartsWith(TEXT("conn"), ESearchCase::IgnoreCase))		{ Out.SplitMode = EMISSplitMode::Connectivity; }
				else if (Value.StartsWith(TEXT("prox"), ESearchCase::IgnoreCase))	{ Out.SplitMode = EMISSplitMode::Proximity; }
				else if (Value.StartsWith(TEXT("mat"), ESearchCase::IgnoreCase))	{ Out.SplitMode = EMISSplitMode::MaterialSlot; }
				else { UE_LOG(LogMISSplitter, Warning, TEXT("Unknown mode '%s'"), *Value); }
			}
			else if (Key.Equals(TEXT("dist"), ESearchCase::IgnoreCase))
			{
				Out.DistanceUnit = EMISDistanceUnit::Absolute;
				Out.MergeDistance = FCString::Atof(*Value);
			}
			else if (Key.Equals(TEXT("pct"), ESearchCase::IgnoreCase))
			{
				Out.DistanceUnit = EMISDistanceUnit::RelativeToBounds;
				Out.MergeDistancePercent = FCString::Atof(*Value);
			}
			else if (Key.Equals(TEXT("weld"), ESearchCase::IgnoreCase))
			{
				const float Tolerance = FCString::Atof(*Value);
				Out.bWeldVertices = Tolerance > 0.0f;
				Out.WeldTolerance = FMath::Max(Tolerance, 0.0f);
			}
			else if (Key.Equals(TEXT("pivot"), ESearchCase::IgnoreCase))
			{
				if (Value.StartsWith(TEXT("orig"), ESearchCase::IgnoreCase))		{ Out.PivotMode = EMISPivotMode::KeepOriginal; }
				else if (Value.StartsWith(TEXT("cent"), ESearchCase::IgnoreCase))	{ Out.PivotMode = EMISPivotMode::BoundsCenter; }
				else if (Value.StartsWith(TEXT("bot"), ESearchCase::IgnoreCase))	{ Out.PivotMode = EMISPivotMode::BoundsBottomCenter; }
				else { UE_LOG(LogMISSplitter, Warning, TEXT("Unknown pivot '%s'"), *Value); }
			}
			else if (Key.Equals(TEXT("matboundary"), ESearchCase::IgnoreCase))
			{
				Out.bRespectMaterialBoundary = FCString::Atoi(*Value) != 0;
			}
			else if (Key.Equals(TEXT("suffix"), ESearchCase::IgnoreCase))
			{
				Out.PartSuffix = Value;
			}
			else if (Key.Equals(TEXT("start"), ESearchCase::IgnoreCase))
			{
				Out.StartIndex = FMath::Max(FCString::Atoi(*Value), 0);
			}
			else if (Key.Equals(TEXT("digits"), ESearchCase::IgnoreCase))
			{
				Out.IndexDigits = FMath::Clamp(FCString::Atoi(*Value), 1, 6);
			}
			else if (Key.Equals(TEXT("folder"), ESearchCase::IgnoreCase))
			{
				Out.OutputSubfolder = Value;
			}
			else if (Key.Equals(TEXT("replace"), ESearchCase::IgnoreCase))
			{
				bOutReplace = FCString::Atoi(*Value) != 0;
			}
			else if (Key.Equals(TEXT("keep"), ESearchCase::IgnoreCase))
			{
				OutReplace.bDeleteOriginalActors = FCString::Atoi(*Value) == 0;
			}
			else
			{
				UE_LOG(LogMISSplitter, Warning, TEXT("Unknown key '%s'"), *Key);
			}
		}
	}

	TArray<UStaticMesh*> GetSelectedStaticMeshes()
	{
		FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FAssetData> Selected;
		ContentBrowser.Get().GetSelectedAssets(Selected);

		TArray<UStaticMesh*> Meshes;
		for (const FAssetData& Asset : Selected)
		{
			if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset()))
			{
				Meshes.Add(Mesh);
			}
		}
		if (Meshes.Num() == 0)
		{
			UE_LOG(LogMISSplitter, Warning, TEXT("Select one or more Static Meshes in the Content Browser first."));
		}
		return Meshes;
	}

	void AnalyzeSelected(const TArray<FString>& Args)
	{
		FMISSplitSettings Settings;
		bool bReplace = false;
		FMISReplaceSettings Replace;
		ParseSettings(Args, Settings, bReplace, Replace);
		MISEditorActions::AnalyzeMeshes(GetSelectedStaticMeshes(), Settings);
	}

	void SplitSelected(const TArray<FString>& Args)
	{
		FMISSplitSettings Settings;
		MISEditorActions::FReplaceRequest Replace;
		bool bReplace = false;
		ParseSettings(Args, Settings, bReplace, Replace.Settings);
		MISEditorActions::SplitMeshes(GetSelectedStaticMeshes(), Settings, bReplace ? &Replace : nullptr);
	}

	void SplitSelectedActors(const TArray<FString>& Args)
	{
		FMISSplitSettings Settings;
		MISEditorActions::FReplaceRequest Replace;
		bool bUnused = false;
		ParseSettings(Args, Settings, bUnused, Replace.Settings);

		if (GEditor)
		{
			GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(Replace.RestrictToActors);
		}
		const TArray<UStaticMesh*> Meshes = MISEditorActions::GetStaticMeshesOfActors(Replace.RestrictToActors);
		if (Meshes.Num() == 0)
		{
			UE_LOG(LogMISSplitter, Warning, TEXT("Select one or more Static Mesh Actors in the level first."));
			return;
		}
		MISEditorActions::SplitMeshes(Meshes, Settings, &Replace);
	}
}

static FAutoConsoleCommand GMISAnalyzeSelectedCommand(
	TEXT("MIS.AnalyzeSelected"),
	TEXT("Dry run of Mesh Island Splitter on selected Static Meshes. Args: mode= dist= pct= weld= matboundary="),
	FConsoleCommandWithArgsDelegate::CreateStatic(&MISConsole::AnalyzeSelected));

static FAutoConsoleCommand GMISSplitSelectedCommand(
	TEXT("MIS.SplitSelected"),
	TEXT("Split selected Static Meshes into new assets. Args: mode= dist= pct= weld= pivot= matboundary= suffix= start= digits= folder= replace= keep="),
	FConsoleCommandWithArgsDelegate::CreateStatic(&MISConsole::SplitSelected));

static FAutoConsoleCommand GMISSplitSelectedActorsCommand(
	TEXT("MIS.SplitSelectedActors"),
	TEXT("Split the Static Meshes of the selected level actors and replace those actors with part actors. Args: mode= dist= pct= weld= pivot= matboundary= suffix= start= digits= folder= keep="),
	FConsoleCommandWithArgsDelegate::CreateStatic(&MISConsole::SplitSelectedActors));
