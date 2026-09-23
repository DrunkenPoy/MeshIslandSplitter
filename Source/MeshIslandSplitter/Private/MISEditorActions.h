// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "MISSplitSettings.h"

class UStaticMesh;

/**
 * 콘텐츠 브라우저 메뉴와 콘솔 커맨드가 공유하는 에디터 액션.
 * 결과는 로그와 에디터 알림(토스트)으로 보고된다.
 */
namespace MISEditorActions
{
	/** 드라이 런: 각 메시의 아일랜드/파트 개수를 보고한다. 아무것도 생성하지 않는다. */
	void AnalyzeMeshes(TConstArrayView<UStaticMesh*> Meshes, const FMISSplitSettings& Settings);

	/** 각 메시를 분할해 원본 옆에 파트 애셋을 생성하고, 생성된 애셋으로 콘텐츠 브라우저를 동기화한다. */
	void SplitMeshes(TConstArrayView<UStaticMesh*> Meshes, const FMISSplitSettings& Settings);
}
