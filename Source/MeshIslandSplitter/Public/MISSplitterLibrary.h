// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MISMeshSplitter.h"
#include "MISSplitterLibrary.generated.h"

/**
 * Editor scripting entry points (Editor Utility Widgets, Python: unreal.MISSplitterLibrary).
 */
UCLASS()
class MESHISLANDSPLITTER_API UMISSplitterLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Dry run: how many parts the settings would produce. Creates nothing. */
	UFUNCTION(BlueprintCallable, Category = "Mesh Island Splitter")
	static bool AnalyzeStaticMesh(UStaticMesh* SourceMesh, const FMISSplitSettings& Settings, FMISSplitAnalysis& OutAnalysis);

	/** Creates one Static Mesh asset per part next to the source. The source is left untouched. */
	UFUNCTION(BlueprintCallable, Category = "Mesh Island Splitter")
	static bool SplitStaticMesh(UStaticMesh* SourceMesh, const FMISSplitSettings& Settings, TArray<FMISSplitPart>& OutParts);
};
