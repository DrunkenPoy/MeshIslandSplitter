// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "MISSplitSettings.generated.h"

UENUM(BlueprintType)
enum class EMISSplitMode : uint8
{
	/** Triangles connected through shared (welded) vertices form one part. Each letter of a text mesh becomes its own part. */
	Connectivity	UMETA(DisplayName = "Connectivity"),

	/** Connectivity islands closer than Merge Distance are merged. A word made of separate letters stays one part. */
	Proximity		UMETA(DisplayName = "Proximity"),

	/** One part per material slot, regardless of topology. */
	MaterialSlot	UMETA(DisplayName = "Material Slot"),
};

UENUM(BlueprintType)
enum class EMISDistanceUnit : uint8
{
	/** Merge distance in centimeters. */
	Absolute			UMETA(DisplayName = "Absolute (cm)"),

	/** Merge distance as a percentage of the source mesh bounds diagonal. */
	RelativeToBounds	UMETA(DisplayName = "Relative to Bounds (%)"),
};

UENUM(BlueprintType)
enum class EMISPivotMode : uint8
{
	/** Keep the source pivot. Parts line up with the source when placed at the same transform. */
	KeepOriginal		UMETA(DisplayName = "Keep Original"),

	/** Move the pivot to the center of each part's bounds. */
	BoundsCenter		UMETA(DisplayName = "Bounds Center"),

	/** Move the pivot to the bottom center of each part's bounds. */
	BoundsBottomCenter	UMETA(DisplayName = "Bounds Bottom Center"),
};

USTRUCT(BlueprintType)
struct MESHISLANDSPLITTER_API FMISSplitSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split")
	EMISSplitMode SplitMode = EMISSplitMode::Proximity;

	/** Treat vertices closer than Weld Tolerance as connected. Only affects grouping; output geometry is not welded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Weld",
		meta = (EditCondition = "SplitMode != EMISSplitMode::MaterialSlot"))
	bool bWeldVertices = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Weld",
		meta = (EditCondition = "bWeldVertices && SplitMode != EMISSplitMode::MaterialSlot", ClampMin = "0.0", UIMax = "1.0", Units = "cm"))
	float WeldTolerance = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Proximity",
		meta = (EditCondition = "SplitMode == EMISSplitMode::Proximity", EditConditionHides))
	EMISDistanceUnit DistanceUnit = EMISDistanceUnit::Absolute;

	/** Islands whose closest triangles are within this distance are merged into one part. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Proximity",
		meta = (EditCondition = "SplitMode == EMISSplitMode::Proximity && DistanceUnit == EMISDistanceUnit::Absolute", EditConditionHides, ClampMin = "0.0", Units = "cm"))
	float MergeDistance = 1.0f;

	/** Merge distance as a percentage of the source bounds diagonal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Proximity",
		meta = (EditCondition = "SplitMode == EMISSplitMode::Proximity && DistanceUnit == EMISDistanceUnit::RelativeToBounds", EditConditionHides, ClampMin = "0.0", ClampMax = "100.0", Units = "Percent"))
	float MergeDistancePercent = 1.0f;

	/** Never merge islands through triangles with different materials. Prevents chains like "gem -> label text". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Proximity",
		meta = (EditCondition = "SplitMode == EMISSplitMode::Proximity", EditConditionHides))
	bool bRespectMaterialBoundary = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	EMISPivotMode PivotMode = EMISPivotMode::KeepOriginal;

	/** New assets are named <Source><PartSuffix><Index>, e.g. SM_Gems_Part_00. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	FString PartSuffix = TEXT("_Part_");
};
