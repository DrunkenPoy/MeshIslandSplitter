// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT
//
// Standalone tests for MISSplitCore. Build: see Tests/CoreStandalone/README.md

#include "MISSplitCore.h"

#include <cstdio>

using namespace MIS;

static int GFailures = 0;

#define EXPECT_EQ(A, B) do { auto _a = (A); auto _b = (B); if (!(_a == _b)) { \
	std::printf("  FAIL %s:%d  %s == %s  (got %lld, expected %lld)\n", __FILE__, __LINE__, #A, #B, (long long)_a, (long long)_b); ++GFailures; } } while (0)
#define EXPECT_NEAR(A, B, Eps) do { double _a = (A); double _b = (B); if (std::fabs(_a - _b) > (Eps)) { \
	std::printf("  FAIL %s:%d  %s ~= %s  (got %.6f, expected %.6f)\n", __FILE__, __LINE__, #A, #B, _a, _b); ++GFailures; } } while (0)

// Axis-aligned box. bWelded=false duplicates vertices per face, like a hard-edged import.
static void AddBox(FCoreMeshInput& M, FVector3d Min, FVector3d Max, int32 Material, bool bWelded)
{
	const FVector3d C[8] = {
		{ Min.X, Min.Y, Min.Z }, { Max.X, Min.Y, Min.Z }, { Max.X, Max.Y, Min.Z }, { Min.X, Max.Y, Min.Z },
		{ Min.X, Min.Y, Max.Z }, { Max.X, Min.Y, Max.Z }, { Max.X, Max.Y, Max.Z }, { Min.X, Max.Y, Max.Z } };
	const int32 Faces[6][4] = { { 0, 3, 2, 1 }, { 4, 5, 6, 7 }, { 0, 1, 5, 4 }, { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };

	int32 Base = M.Positions.Num();
	if (bWelded)
	{
		for (const FVector3d& P : C) M.Positions.Add(P);
	}
	for (const auto& F : Faces)
	{
		int32 Idx[4];
		for (int32 K = 0; K < 4; ++K)
		{
			if (bWelded) { Idx[K] = Base + F[K]; }
			else { Idx[K] = M.Positions.Add(C[F[K]]); }
		}
		const int32 Tris[2][3] = { { Idx[0], Idx[1], Idx[2] }, { Idx[0], Idx[2], Idx[3] } };
		for (const auto& T : Tris)
		{
			M.TriangleVertices.Add(T[0]); M.TriangleVertices.Add(T[1]); M.TriangleVertices.Add(T[2]);
			M.TriangleMaterials.Add(Material);
		}
	}
}

// A "word": one box per letter, letters with a dot get an extra small box above.
static void AddWord(FCoreMeshInput& M, double StartX, double Z, int32 Letters, const TArray<int32>& DottedLetters, int32 Material)
{
	const double W = 2.0, Gap = 0.5, H = 3.0;
	for (int32 L = 0; L < Letters; ++L)
	{
		const double X = StartX + L * (W + Gap);
		AddBox(M, { X, 0, Z }, { X + W, 1, Z + H }, Material, true);
	}
	for (const int32 L : DottedLetters)
	{
		const double X = StartX + L * (W + Gap);
		AddBox(M, { X + 0.5, 0, Z + H + 0.4 }, { X + 1.5, 1, Z + H + 1.4 }, Material, true);
	}
}

// Layout mirroring the reference image: two gems above two words.
static FCoreMeshInput MakeReferenceScene()
{
	FCoreMeshInput M;
	AddBox(M, { 0, 0, 0 }, { 10, 10, 10 }, 0, false);	// 1: cushion gem, faceted (unwelded)
	AddBox(M, { 25, 0, 0 }, { 35, 10, 10 }, 1, true);	// 2: trillion gem
	TArray<int32> CushionDots; CushionDots.Add(4);		// C u s h [i] o n
	AddWord(M, 0.0, -10.0, 7, CushionDots, 2);			// 3: "Cushion"  (x 0..17, top z -5.6)
	TArray<int32> TrillionDots; TrillionDots.Add(3); TrillionDots.Add(5); // T r i [l] l [i]... indices approximate
	AddWord(M, 22.0, -10.0, 8, TrillionDots, 2);		// 4: "Trillion" (starts 5cm after word 3)
	return M;
}

static void TestTriangleDistance()
{
	std::printf("TriangleTriangleDistance\n");
	const FVector3d A[3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 } };

	const FVector3d Parallel[3] = { { 0, 0, 3 }, { 1, 0, 3 }, { 0, 1, 3 } };
	EXPECT_NEAR(TriangleTriangleDistanceSq(A, Parallel), 9.0, 1e-9);

	const FVector3d Piercing[3] = { { 0.2, 0.2, -1 }, { 0.2, 0.2, 1 }, { 0.3, 0.25, 1 } };
	EXPECT_NEAR(TriangleTriangleDistanceSq(A, Piercing), 0.0, 1e-12);

	// Edge-edge: skew edge 2 units above the hypotenuse region, crossing it
	const FVector3d Skew[3] = { { 0.5, -1, 2 }, { 0.5, 2, 2 }, { 0.5, 2, 5 } };
	EXPECT_NEAR(TriangleTriangleDistanceSq(A, Skew), 4.0, 1e-9);

	// Degenerate (collinear) triangle 1 unit beside A's edge
	const FVector3d Degen[3] = { { 0, -1, 0 }, { 1, -1, 0 }, { 0.5, -1, 0 } };
	EXPECT_NEAR(TriangleTriangleDistanceSq(A, Degen), 1.0, 1e-9);
}

static void TestReferenceScene()
{
	const FCoreMeshInput M = MakeReferenceScene();
	FCoreSplitResult R;
	FCoreSplitParams P;

	std::printf("Connectivity without weld\n");
	P.Mode = ECoreSplitMode::Connectivity; P.bWeldVertices = false;
	ComputeSplitGroups(M, P, R);
	// gem1: 6 faces, gem2: 1, Cushion: 7+1, Trillion: 8+2
	EXPECT_EQ(R.NumGroups, 6 + 1 + 8 + 10);

	std::printf("Connectivity with weld\n");
	P.bWeldVertices = true; P.WeldTolerance = 0.01;
	ComputeSplitGroups(M, P, R);
	EXPECT_EQ(R.NumIslands, 1 + 1 + 8 + 10);
	EXPECT_EQ(R.NumGroups, 20);

	std::printf("Proximity 1cm -> 4 parts like the reference image\n");
	P.Mode = ECoreSplitMode::Proximity; P.MergeDistance = 1.0;
	ComputeSplitGroups(M, P, R);
	EXPECT_EQ(R.NumIslands, 20);
	EXPECT_EQ(R.NumGroups, 4);
	// Sanity: first triangle (gem1) and last triangle (Trillion) in different groups
	EXPECT_EQ(R.TriangleGroups[0] != R.TriangleGroups[M.NumTriangles() - 1], true);

	std::printf("Proximity 0.3cm -> letters stay apart\n");
	P.MergeDistance = 0.3;
	ComputeSplitGroups(M, P, R);
	EXPECT_EQ(R.NumGroups, 20);

	std::printf("Proximity 6cm -> chain merges gems with words\n");
	P.MergeDistance = 6.0;
	ComputeSplitGroups(M, P, R);
	EXPECT_EQ(R.NumGroups, 1);

	std::printf("Proximity 6cm + respect material -> gems stay separate\n");
	P.bRespectMaterialBoundary = true;
	ComputeSplitGroups(M, P, R);
	EXPECT_EQ(R.NumGroups, 3); // gem1, gem2, both words (same material, 5cm apart)

	std::printf("Material slot\n");
	P = FCoreSplitParams(); P.Mode = ECoreSplitMode::MaterialSlot;
	ComputeSplitGroups(M, P, R);
	EXPECT_EQ(R.NumGroups, 3);

	std::printf("Bounds diagonal\n");
	EXPECT_EQ(ComputeBoundsDiagonal(M) > 40.0, true);
}

int main()
{
	TestTriangleDistance();
	TestReferenceScene();
	std::printf(GFailures == 0 ? "\nALL PASSED\n" : "\n%d FAILURE(S)\n", GFailures);
	return GFailures == 0 ? 0 : 1;
}
