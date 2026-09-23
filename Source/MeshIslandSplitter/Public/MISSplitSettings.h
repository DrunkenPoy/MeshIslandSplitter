// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "MISSplitSettings.generated.h"

UENUM(BlueprintType)
enum class EMISSplitMode : uint8
{
	/** 공유(용접된) 정점으로 연결된 삼각형들이 하나의 파트를 이룬다. 텍스트 메시의 각 글자가 각각의 파트가 된다. */
	Connectivity	UMETA(DisplayName = "Connectivity"),

	/** Merge Distance보다 가까운 연결성 아일랜드들을 병합한다. 여러 글자로 이루어진 단어는 하나의 파트로 유지된다. */
	Proximity		UMETA(DisplayName = "Proximity"),

	/** 토폴로지와 무관하게 머티리얼 슬롯당 하나의 파트. */
	MaterialSlot	UMETA(DisplayName = "Material Slot"),
};

UENUM(BlueprintType)
enum class EMISDistanceUnit : uint8
{
	/** 병합 거리를 센티미터 단위로 지정. */
	Absolute			UMETA(DisplayName = "Absolute (cm)"),

	/** 병합 거리를 원본 메시 바운즈 대각선 대비 백분율로 지정. */
	RelativeToBounds	UMETA(DisplayName = "Relative to Bounds (%)"),
};

UENUM(BlueprintType)
enum class EMISPivotMode : uint8
{
	/** 원본 피벗을 유지한다. 동일한 트랜스폼에 배치하면 파트들이 원본과 정렬된다. */
	KeepOriginal		UMETA(DisplayName = "Keep Original"),

	/** 각 파트 바운즈의 중심으로 피벗을 이동한다. */
	BoundsCenter		UMETA(DisplayName = "Bounds Center"),

	/** 각 파트 바운즈의 바닥 중심으로 피벗을 이동한다. */
	BoundsBottomCenter	UMETA(DisplayName = "Bounds Bottom Center"),
};

USTRUCT(BlueprintType)
struct MESHISLANDSPLITTER_API FMISSplitSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split")
	EMISSplitMode SplitMode = EMISSplitMode::Proximity;

	/** Weld Tolerance보다 가까운 정점들을 연결된 것으로 취급한다. 그룹화에만 영향을 주며 출력 지오메트리는 용접되지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Weld",
		meta = (EditCondition = "SplitMode != EMISSplitMode::MaterialSlot"))
	bool bWeldVertices = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Weld",
		meta = (EditCondition = "bWeldVertices && SplitMode != EMISSplitMode::MaterialSlot", ClampMin = "0.0", UIMax = "1.0", Units = "cm"))
	float WeldTolerance = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Proximity",
		meta = (EditCondition = "SplitMode == EMISSplitMode::Proximity", EditConditionHides))
	EMISDistanceUnit DistanceUnit = EMISDistanceUnit::Absolute;

	/** 최근접 삼각형 사이 거리가 이 값 이내인 아일랜드들은 하나의 파트로 병합된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Proximity",
		meta = (EditCondition = "SplitMode == EMISSplitMode::Proximity && DistanceUnit == EMISDistanceUnit::Absolute", EditConditionHides, ClampMin = "0.0", Units = "cm"))
	float MergeDistance = 1.0f;

	/** 원본 바운즈 대각선 대비 백분율로 지정하는 병합 거리. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Proximity",
		meta = (EditCondition = "SplitMode == EMISSplitMode::Proximity && DistanceUnit == EMISDistanceUnit::RelativeToBounds", EditConditionHides, ClampMin = "0.0", ClampMax = "100.0", Units = "Percent"))
	float MergeDistancePercent = 1.0f;

	/** 서로 다른 머티리얼을 가진 삼각형을 통해서는 아일랜드를 병합하지 않는다. "보석 -> 라벨 텍스트" 같은 연쇄 병합을 방지한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split|Proximity",
		meta = (EditCondition = "SplitMode == EMISSplitMode::Proximity", EditConditionHides))
	bool bRespectMaterialBoundary = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	EMISPivotMode PivotMode = EMISPivotMode::KeepOriginal;

	/** 새 애셋은 <원본><PartSuffix><인덱스> 형식으로 이름이 지정된다, 예: SM_Gems_Part_00. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Naming")
	FString PartSuffix = TEXT("_Part_");

	/** 첫 파트의 인덱스. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Naming", meta = (ClampMin = "0"))
	int32 StartIndex = 0;

	/** 인덱스 자릿수 (0으로 채움). 2이면 _00, _01, ... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Naming", meta = (ClampMin = "1", ClampMax = "6"))
	int32 IndexDigits = 2;

	/** 원본 폴더 아래 파트를 저장할 하위 폴더 (예: "Parts"). 비우면 원본과 같은 폴더. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Naming")
	FString OutputSubfolder;
};

/** 레벨에 배치된 원본 액터를 분할 파트로 치환할 때의 옵션. */
USTRUCT(BlueprintType)
struct MESHISLANDSPLITTER_API FMISReplaceSettings
{
	GENERATED_BODY()

	/** 치환 후 원본 액터를 삭제한다. 끄면 원본은 그대로 남는다. (Undo 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replace")
	bool bDeleteOriginalActors = true;

	/** 파트 액터들을 원본 액터 라벨 이름의 아웃라이너 폴더로 묶는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replace")
	bool bGroupInOutlinerFolder = true;
};
