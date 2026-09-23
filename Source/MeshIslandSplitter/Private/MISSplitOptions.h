// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "MISSplitSettings.h"
#include "UObject/Object.h"
#include "MISSplitOptions.generated.h"

/**
 * 옵션 다이얼로그가 편집하는 설정 (4단계). CDO를 그대로 편집하며
 * 프로젝트별 사용자 설정(EditorPerProjectUserSettings.ini)에 저장되어 다음 세션에도 유지된다.
 */
UCLASS(config = EditorPerProjectUserSettings)
class UMISSplitOptions : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Config, Category = "Split Settings", meta = (ShowOnlyInnerProperties))
	FMISSplitSettings Split;

	/** 분할 후 현재 레벨에서 원본 메시를 사용하는 StaticMeshActor를 모두 파트 액터들로 치환한다. */
	UPROPERTY(EditAnywhere, Config, Category = "Level Actors")
	bool bReplaceActorsInLevel = false;

	UPROPERTY(EditAnywhere, Config, Category = "Level Actors", meta = (ShowOnlyInnerProperties))
	FMISReplaceSettings Replace;

	static UMISSplitOptions& Get() { return *GetMutableDefault<UMISSplitOptions>(); }
};
