// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

namespace MISSplitDialog
{
	enum class EScope : uint8
	{
		/** 콘텐츠 브라우저에서 선택한 애셋 또는 폴더. 레벨 치환은 옵션(bReplaceActorsInLevel)을 따른다. */
		Assets,

		/** 레벨에서 선택한 액터. 선택한 액터는 항상 치환되므로 bReplaceActorsInLevel 옵션을 숨긴다. */
		SelectedActors,
	};

	/**
	 * 분할 옵션 다이얼로그를 모달로 띄운다. 옵션은 UMISSplitOptions에 바로 반영되고 저장된다.
	 * 다이얼로그 안의 Analyze 버튼으로 현재 옵션의 결과를 미리 볼 수 있다.
	 * @return 사용자가 Split을 눌렀으면 true.
	 */
	bool Show(const FText& Description, TConstArrayView<UStaticMesh*> Meshes, EScope Scope);
}
