// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "MISSplitSettings.h"
#include "MISMeshSplitter.generated.h"

class UStaticMesh;

DECLARE_LOG_CATEGORY_EXTERN(LogMISSplitter, Log, All);

/** 생성된 파트 하나. PivotOffset은 원본 공간 기준 새 피벗의 위치이다 (KeepOriginal이면 0). */
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

/** 드라이 런 결과. 애셋을 생성하기 전 Merge Distance를 조정하는 데 사용한다. */
USTRUCT(BlueprintType)
struct MESHISLANDSPLITTER_API FMISSplitAnalysis
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	int32 TriangleCount = 0;

	/** 용접 이후의 연결성 아일랜드 수. */
	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	int32 IslandCount = 0;

	/** 현재 설정으로 생성될 파트 수. */
	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	int32 PartCount = 0;

	/** 상대 단위를 변환한 후의 병합 거리 (cm). */
	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	float ResolvedMergeDistance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Mesh Island Splitter")
	TArray<int32> PartTriangleCounts;
};

class MESHISLANDSPLITTER_API FMISMeshSplitter
{
public:
	/** 애셋을 생성하지 않고 분할 결과만 계산한다. LOD0을 사용한다. */
	static bool Analyze(const UStaticMesh* Source, const FMISSplitSettings& Settings, FMISSplitAnalysis& OutAnalysis, FText* OutError = nullptr);

	/**
	 * 원본 애셋 옆에 파트마다 새 스태틱 메시 애셋을 하나씩 생성한다. 원본은 수정되지 않는다.
	 * LOD0만 분할된다. 결과 파트가 2개 미만이면 false를 반환하며 아무것도 생성하지 않는다.
	 */
	static bool Split(UStaticMesh* Source, const FMISSplitSettings& Settings, TArray<FMISSplitPart>& OutParts, FText* OutError = nullptr);
};
