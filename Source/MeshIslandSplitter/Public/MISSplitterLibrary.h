// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MISMeshSplitter.h"
#include "MISSplitterLibrary.generated.h"

class AActor;
class AStaticMeshActor;

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

	/**
	 * 일괄 처리: 각 메시를 분할하고 생성된 모든 파트를 OutParts에 모은다.
	 * 분할할 것이 없는 메시는 건너뛴다. 실제로 분할된 메시 수를 반환한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh Island Splitter")
	static int32 SplitStaticMeshes(const TArray<UStaticMesh*>& SourceMeshes, const FMISSplitSettings& Settings, TArray<FMISSplitPart>& OutParts);

	/** 현재 에디터 레벨에서 SourceMesh를 사용하는 (치환 가능한) Static Mesh Actor 목록. */
	UFUNCTION(BlueprintCallable, Category = "Mesh Island Splitter")
	static TArray<AStaticMeshActor*> GetLevelActorsUsingMesh(UStaticMesh* SourceMesh);

	/**
	 * 원본 액터들을 파트 액터로 치환한다 (Undo 가능). Parts는 SplitStaticMesh(SourceMesh)의 결과여야 한다.
	 * 치환된 원본 액터 수를 반환한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh Island Splitter")
	static int32 ReplaceActorsWithParts(UStaticMesh* SourceMesh, const TArray<FMISSplitPart>& Parts, const TArray<AStaticMeshActor*>& Actors,
		const FMISReplaceSettings& Settings, TArray<AActor*>& OutNewActors);
};
