// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "MISSplitSettings.h"
#include "MISMeshSplitter.generated.h"

class UStaticMesh;

DECLARE_LOG_CATEGORY_EXTERN(LogMISSplitter, Log, All);

/** One generated part. PivotOffset is the source-space position of the new pivot (zero for KeepOriginal). */
USTRUCT(BlueprintType)
struct MESHISLANDSPLITTER_API FMISSplitPart
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	FVector PivotOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	int32 TriangleCount = 0;
};

/** Result of a dry run. Use it to tune Merge Distance before creating assets. */
USTRUCT(BlueprintType)
struct MESHISLANDSPLITTER_API FMISSplitAnalysis
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	int32 TriangleCount = 0;

	/** Connectivity islands after welding. */
	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	int32 IslandCount = 0;

	/** Parts the current settings would produce. */
	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	int32 PartCount = 0;

	/** Merge distance in cm after resolving relative units. */
	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	float ResolvedMergeDistance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	TArray<int32> PartTriangleCounts;
};

class MESHISLANDSPLITTER_API FMISMeshSplitter
{
public:
	/** Computes the split without creating assets. Uses LOD0. */
	static bool Analyze(const UStaticMesh* Source, const FMISSplitSettings& Settings, FMISSplitAnalysis& OutAnalysis, FText* OutError = nullptr);

	/**
	 * Creates one new Static Mesh asset per part next to the source asset. The source is not modified.
	 * Only LOD0 is split. Returns false (and creates nothing) if the mesh would produce fewer than two parts.
	 */
	static bool Split(UStaticMesh* Source, const FMISSplitSettings& Settings, TArray<FMISSplitPart>& OutParts, FText* OutError = nullptr);
};
