// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "MISSplitSettings.h"

class AActor;
class UStaticMesh;

/**
 * 콘텐츠 브라우저·레벨 에디터 메뉴와 콘솔 커맨드가 공유하는 에디터 액션.
 * 결과는 로그와 에디터 알림(토스트)으로 보고된다.
 */
namespace MISEditorActions
{
	/** 분할 후 레벨 액터 치환 요청 (5단계). */
	struct FReplaceRequest
	{
		FMISReplaceSettings Settings;

		/** 비어 있으면 에디터 월드에서 원본 메시를 쓰는 모든 액터를, 아니면 이 액터들만 치환한다. */
		TArray<AActor*> RestrictToActors;
	};

	/** 드라이 런: 각 메시의 아일랜드/파트 개수를 보고한다. 아무것도 생성하지 않는다. */
	void AnalyzeMeshes(TConstArrayView<UStaticMesh*> Meshes, const FMISSplitSettings& Settings);

	/**
	 * 각 메시를 분할해 파트 애셋을 생성하고, 생성된 애셋으로 콘텐츠 브라우저를 동기화한다.
	 * Replace가 주어지면 레벨의 원본 액터를 파트 액터로 치환하고 새 액터를 선택한다.
	 * 여러 메시를 일괄 처리할 수 있으며 진행 창에서 취소할 수 있다. 분할할 것이 없는 메시는 건너뛴다.
	 */
	void SplitMeshes(TConstArrayView<UStaticMesh*> Meshes, const FMISSplitSettings& Settings, const FReplaceRequest* Replace = nullptr);

	/** 패키지 경로(하위 폴더 포함) 아래의 모든 스태틱 메시를 로드한다. 일괄 처리용. */
	TArray<UStaticMesh*> LoadStaticMeshesInPaths(TConstArrayView<FString> PackagePaths);

	/** 액터들이 사용하는 스태틱 메시 (중복 제거, 등장 순서 유지). 치환 가능한 AStaticMeshActor만 고려한다. */
	TArray<UStaticMesh*> GetStaticMeshesOfActors(TConstArrayView<AActor*> Actors);

	// 메뉴 진입점. 옵션 다이얼로그(4단계)를 띄우거나 저장된 옵션을 그대로 사용한다.
	void AnalyzeWithSavedOptions(TConstArrayView<UStaticMesh*> Meshes);
	void SplitWithSavedOptions(TConstArrayView<UStaticMesh*> Meshes);
	void OpenDialogForAssets(TConstArrayView<UStaticMesh*> Meshes);
	void OpenDialogForFolders(TConstArrayView<FString> PackagePaths);
	void OpenDialogForActors(TConstArrayView<AActor*> Actors);
}
