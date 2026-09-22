// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

/**
 * Engine-agnostic split core.
 *
 * Only uses TArray / TMap / FVector3d / FMath so it can be compiled and unit-tested
 * outside the engine (see Tests/CoreStandalone). Everything that touches UObjects or
 * FMeshDescription lives in MISMeshSplitter.cpp.
 */
namespace MIS
{
	enum class ECoreSplitMode : uint8
	{
		Connectivity,	// Triangles sharing a (welded) vertex belong together
		Proximity,		// Connectivity islands closer than MergeDistance are merged
		MaterialSlot	// One group per material slot, topology ignored
	};

	struct FCoreMeshInput
	{
		TArray<FVector3d> Positions;
		TArray<int32> TriangleVertices;		// 3 indices into Positions per triangle
		TArray<int32> TriangleMaterials;	// 1 material id per triangle

		int32 NumTriangles() const { return TriangleVertices.Num() / 3; }
	};

	struct FCoreSplitParams
	{
		ECoreSplitMode Mode = ECoreSplitMode::Proximity;
		bool bWeldVertices = true;
		double WeldTolerance = 0.01;	// cm
		double MergeDistance = 1.0;		// cm, already resolved to absolute units
		bool bRespectMaterialBoundary = false;
	};

	struct FCoreSplitResult
	{
		TArray<int32> TriangleGroups;	// group index per triangle, 0..NumGroups-1
		int32 NumGroups = 0;
		int32 NumIslands = 0;			// connectivity islands (after welding), for diagnostics
	};

	/** Assigns every triangle to a group. Groups are numbered in order of first appearance. */
	void ComputeSplitGroups(const FCoreMeshInput& Input, const FCoreSplitParams& Params, FCoreSplitResult& OutResult);

	/** Diagonal length of the bounds of all referenced vertices. Used for relative distances. */
	double ComputeBoundsDiagonal(const FCoreMeshInput& Input);

	/** Squared minimum distance between two triangles (0 if they intersect). Exposed for tests. */
	double TriangleTriangleDistanceSq(const FVector3d A[3], const FVector3d B[3]);
}
