// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

/**
 * 엔진에 독립적인 분할 코어.
 *
 * TArray / TMap / FVector3d / FMath만 사용하므로 엔진 없이도
 * 컴파일하고 단위 테스트할 수 있다 (Tests/CoreStandalone 참고). UObject나
 * FMeshDescription을 다루는 모든 코드는 MISMeshSplitter.cpp에 있다.
 */
namespace MIS
{
	enum class ECoreSplitMode : uint8
	{
		Connectivity,	// (용접된) 정점을 공유하는 삼각형들은 같은 그룹으로 묶임
		Proximity,		// MergeDistance보다 가까운 연결성 아일랜드들을 병합
		MaterialSlot	// 토폴로지와 무관하게 머티리얼 슬롯당 하나의 그룹
	};

	struct FCoreMeshInput
	{
		TArray<FVector3d> Positions;
		TArray<int32> TriangleVertices;		// 삼각형당 Positions를 가리키는 인덱스 3개
		TArray<int32> TriangleMaterials;	// 삼각형당 머티리얼 id 1개

		int32 NumTriangles() const { return TriangleVertices.Num() / 3; }
	};

	struct FCoreSplitParams
	{
		ECoreSplitMode Mode = ECoreSplitMode::Proximity;
		bool bWeldVertices = true;
		double WeldTolerance = 0.01;	// cm
		double MergeDistance = 1.0;		// cm, 이미 절대 단위로 변환된 값
		bool bRespectMaterialBoundary = false;
	};

	struct FCoreSplitResult
	{
		TArray<int32> TriangleGroups;	// 삼각형당 그룹 인덱스, 0..NumGroups-1
		int32 NumGroups = 0;
		int32 NumIslands = 0;			// 용접 이후의 연결성 아일랜드 수 (진단용)
	};

	/** 모든 삼각형에 그룹을 할당한다. 그룹 번호는 처음 등장한 순서대로 매겨진다. */
	void ComputeSplitGroups(const FCoreMeshInput& Input, const FCoreSplitParams& Params, FCoreSplitResult& OutResult);

	/** 참조되는 모든 정점의 바운즈 대각선 길이. 상대 거리 계산에 사용된다. */
	double ComputeBoundsDiagonal(const FCoreMeshInput& Input);

	/** 두 삼각형 사이 최소 거리의 제곱 (교차하면 0). 테스트를 위해 노출됨. */
	double TriangleTriangleDistanceSq(const FVector3d A[3], const FVector3d B[3]);
}
