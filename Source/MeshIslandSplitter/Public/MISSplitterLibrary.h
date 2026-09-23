// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MISMeshSplitter.h"
#include "MISSplitterLibrary.generated.h"

/**
 * 에디터 스크립팅 진입점 (Editor Utility Widgets, Python: unreal.MISSplitterLibrary).
 */
UCLASS()
class MESHISLANDSPLITTER_API UMISSplitterLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 드라이 런: 현재 설정으로 몇 개의 파트가 생성될지 계산한다. 아무것도 생성하지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Mesh Island Splitter")
	static bool AnalyzeStaticMesh(UStaticMesh* SourceMesh, const FMISSplitSettings& Settings, FMISSplitAnalysis& OutAnalysis);

	/** 원본 옆에 파트마다 스태틱 메시 애셋을 하나씩 생성한다. 원본은 변경되지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Mesh Island Splitter")
	static bool SplitStaticMesh(UStaticMesh* SourceMesh, const FMISSplitSettings& Settings, TArray<FMISSplitPart>& OutParts);
};
