// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT
//
// 개발용 콘솔 커맨드 (3단계에서 콘텐츠 브라우저 메뉴가 추가되기 전까지 사용).
//
//   MIS.AnalyzeSelected [key=value ...]   드라이 런, 아일랜드/파트 개수를 로그로 출력
//   MIS.SplitSelected   [key=value ...]   선택한 각 스태틱 메시 옆에 파트 애셋 생성
//
// 키:
//   mode=connectivity|proximity|material   (기본값 proximity)
//   dist=<cm>          절대 병합 거리
//   pct=<percent>      바운즈 대각선 대비 상대 병합 거리
//   weld=<cm>          용접 허용 오차, 0이면 용접 비활성화
//   pivot=original|center|bottom
//   matboundary=0|1    머티리얼 경계를 넘어 병합하지 않음
//
// 예: MIS.AnalyzeSelected mode=proximity dist=1.5 matboundary=1

#include "MISMeshSplitter.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"

namespace MISConsole
{
	void ParseSettings(const TArray<FString>& Args, FMISSplitSettings& Out)
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
		ParseSettings(Args, Settings);
		for (UStaticMesh* Mesh : GetSelectedStaticMeshes())
		{
			FMISSplitAnalysis Analysis;
			if (FMISMeshSplitter::Analyze(Mesh, Settings, Analysis))
			{
				FString Counts;
				for (int32 Index = 0; Index < Analysis.PartTriangleCounts.Num(); ++Index)
				{
					Counts += FString::Printf(TEXT("%s%d"), Index ? TEXT(", ") : TEXT(""), Analysis.PartTriangleCounts[Index]);
				}
				UE_LOG(LogMISSplitter, Display, TEXT("  part triangle counts: [%s]"), *Counts);
			}
		}
	}

	void SplitSelected(const TArray<FString>& Args)
	{
		FMISSplitSettings Settings;
		ParseSettings(Args, Settings);

		TArray<UObject*> Created;
		for (UStaticMesh* Mesh : GetSelectedStaticMeshes())
		{
			TArray<FMISSplitPart> Parts;
			if (FMISMeshSplitter::Split(Mesh, Settings, Parts))
			{
				for (const FMISSplitPart& Part : Parts)
				{
					Created.Add(Part.Mesh);
				}
			}
		}

		if (Created.Num() > 0)
		{
			FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowser.Get().SyncBrowserToAssets(Created);
		}
	}
}

static FAutoConsoleCommand GMISAnalyzeSelectedCommand(
	TEXT("MIS.AnalyzeSelected"),
	TEXT("Dry run of Mesh Island Splitter on selected Static Meshes. Args: mode= dist= pct= weld= pivot= matboundary="),
	FConsoleCommandWithArgsDelegate::CreateStatic(&MISConsole::AnalyzeSelected));

static FAutoConsoleCommand GMISSplitSelectedCommand(
	TEXT("MIS.SplitSelected"),
	TEXT("Split selected Static Meshes into new assets. Args: mode= dist= pct= weld= pivot= matboundary="),
	FConsoleCommandWithArgsDelegate::CreateStatic(&MISConsole::SplitSelected));
