// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT
//
// MISSplitCore를 위한 독립 실행 테스트. 빌드 방법: Tests/CoreStandalone/README.md 참고

#include "MISSplitCore.h"

#include <cstdio>

using namespace MIS;

static int GFailures = 0;

#define EXPECT_EQ(A, B) do { auto _a = (A); auto _b = (B); if (!(_a == _b)) { \
	std::printf("  FAIL %s:%d  %s == %s  (got %lld, expected %lld)\n", __FILE__, __LINE__, #A, #B, (long long)_a, (long long)_b); ++GFailures; } } while (0)
#define EXPECT_NEAR(A, B, Eps) do { double _a = (A); double _b = (B); if (std::fabs(_a - _b) > (Eps)) { \
	std::printf("  FAIL %s:%d  %s ~= %s  (got %.6f, expected %.6f)\n", __FILE__, __LINE__, #A, #B, _a, _b); ++GFailures; } } while (0)

// 축 정렬 박스. bWelded=false이면 하드 엣지로 임포트된 것처럼 면마다 정점을 복제한다.
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

// "단어": 글자마다 박스 하나씩, 점이 있는 글자는 위에 작은 박스를 추가로 둔다.
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

// 참고 이미지와 동일한 배치: 두 개의 보석이 두 단어 위에 위치.
static FCoreMeshInput MakeReferenceScene()
{
	FCoreMeshInput M;
	AddBox(M, { 0, 0, 0 }, { 10, 10, 10 }, 0, false);	// 1: 쿠션 보석, 페이싯 처리 (용접 안 됨)
	AddBox(M, { 25, 0, 0 }, { 35, 10, 10 }, 1, true);	// 2: 트릴리언 보석
	TArray<int32> CushionDots; CushionDots.Add(4);		// C u s h [i] o n
	AddWord(M, 0.0, -10.0, 7, CushionDots, 2);			// 3: "Cushion"  (x 0..17, top z -5.6)
	TArray<int32> TrillionDots; TrillionDots.Add(3); TrillionDots.Add(5); // T r i [l] l [i]... 인덱스는 근사치
	AddWord(M, 22.0, -10.0, 8, TrillionDots, 2);		// 4: "Trillion" (단어 3 이후 5cm 지점에서 시작)
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

	// 엣지-엣지: 빗변 영역 위 2 유닛 지점을 가로지르는 스큐 엣지
	const FVector3d Skew[3] = { { 0.5, -1, 2 }, { 0.5, 2, 2 }, { 0.5, 2, 5 } };
	EXPECT_NEAR(TriangleTriangleDistanceSq(A, Skew), 4.0, 1e-9);

	// 퇴화된(공선) 삼각형, A의 엣지에서 1 유닛 옆
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
	// gem1: 6면, gem2: 1, Cushion: 7+1, Trillion: 8+2
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
	// 검증: 첫 번째 삼각형(gem1)과 마지막 삼각형(Trillion)이 서로 다른 그룹에 속함
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
	EXPECT_EQ(R.NumGroups, 3); // gem1, gem2, 두 단어 모두 (같은 머티리얼, 5cm 간격)

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
