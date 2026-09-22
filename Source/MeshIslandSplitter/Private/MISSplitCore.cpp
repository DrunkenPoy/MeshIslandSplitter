// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT

#include "MISSplitCore.h"

namespace MIS
{
namespace CoreImpl
{
	// ------------------------------------------------------------------
	// Union-Find (path halving, union toward the smaller index so that
	// roots stay deterministic regardless of merge order)
	// ------------------------------------------------------------------
	struct FUnionFind
	{
		TArray<int32> Parent;

		explicit FUnionFind(int32 Count)
		{
			Parent.SetNumUninitialized(Count);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				Parent[Index] = Index;
			}
		}

		int32 Find(int32 X)
		{
			while (Parent[X] != X)
			{
				Parent[X] = Parent[Parent[X]];
				X = Parent[X];
			}
			return X;
		}

		bool Union(int32 A, int32 B)
		{
			A = Find(A);
			B = Find(B);
			if (A == B)
			{
				return false;
			}
			if (A < B)
			{
				Parent[B] = A;
			}
			else
			{
				Parent[A] = B;
			}
			return true;
		}
	};

	// ------------------------------------------------------------------
	// Small AABB
	// ------------------------------------------------------------------
	struct FAabb
	{
		FVector3d Min = FVector3d(TNumericLimits<double>::Max(), TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
		FVector3d Max = FVector3d(-TNumericLimits<double>::Max(), -TNumericLimits<double>::Max(), -TNumericLimits<double>::Max());

		void Add(const FVector3d& P)
		{
			Min = FVector3d(FMath::Min(Min.X, P.X), FMath::Min(Min.Y, P.Y), FMath::Min(Min.Z, P.Z));
			Max = FVector3d(FMath::Max(Max.X, P.X), FMath::Max(Max.Y, P.Y), FMath::Max(Max.Z, P.Z));
		}

		void Add(const FAabb& Other)
		{
			Add(Other.Min);
			Add(Other.Max);
		}

		bool IsValid() const
		{
			return Min.X <= Max.X;
		}

		/** True if the boxes overlap after growing this box by Margin on every side. */
		bool OverlapsWithMargin(const FAabb& Other, double Margin) const
		{
			return !(Min.X - Margin > Other.Max.X || Other.Min.X - Margin > Max.X
				|| Min.Y - Margin > Other.Max.Y || Other.Min.Y - Margin > Max.Y
				|| Min.Z - Margin > Other.Max.Z || Other.Min.Z - Margin > Max.Z);
		}
	};

	// ------------------------------------------------------------------
	// Geometry helpers
	// ------------------------------------------------------------------
	inline double Dot(const FVector3d& A, const FVector3d& B)
	{
		return FVector3d::DotProduct(A, B);
	}

	inline FVector3d Cross(const FVector3d& A, const FVector3d& B)
	{
		return FVector3d::CrossProduct(A, B);
	}

	/** Closest point on a non-degenerate triangle (Ericson, Real-Time Collision Detection 5.1.5). */
	FVector3d ClosestPointOnTriangle(const FVector3d& P, const FVector3d& A, const FVector3d& B, const FVector3d& C)
	{
		const FVector3d AB = B - A;
		const FVector3d AC = C - A;
		const FVector3d AP = P - A;
		const double D1 = Dot(AB, AP);
		const double D2 = Dot(AC, AP);
		if (D1 <= 0.0 && D2 <= 0.0)
		{
			return A;
		}

		const FVector3d BP = P - B;
		const double D3 = Dot(AB, BP);
		const double D4 = Dot(AC, BP);
		if (D3 >= 0.0 && D4 <= D3)
		{
			return B;
		}

		const double VC = D1 * D4 - D3 * D2;
		if (VC <= 0.0 && D1 >= 0.0 && D3 <= 0.0)
		{
			const double V = D1 / (D1 - D3);
			return A + AB * V;
		}

		const FVector3d CP = P - C;
		const double D5 = Dot(AB, CP);
		const double D6 = Dot(AC, CP);
		if (D6 >= 0.0 && D5 <= D6)
		{
			return C;
		}

		const double VB = D5 * D2 - D1 * D6;
		if (VB <= 0.0 && D2 >= 0.0 && D6 <= 0.0)
		{
			const double W = D2 / (D2 - D6);
			return A + AC * W;
		}

		const double VA = D3 * D6 - D5 * D4;
		if (VA <= 0.0 && (D4 - D3) >= 0.0 && (D5 - D6) >= 0.0)
		{
			const double W = (D4 - D3) / ((D4 - D3) + (D5 - D6));
			return B + (C - B) * W;
		}

		const double Denom = 1.0 / (VA + VB + VC);
		const double V = VB * Denom;
		const double W = VC * Denom;
		return A + AB * V + AC * W;
	}

	/** Squared distance between segments P1-Q1 and P2-Q2 (Ericson 5.1.9). Handles degenerate segments. */
	double SegmentSegmentDistanceSq(const FVector3d& P1, const FVector3d& Q1, const FVector3d& P2, const FVector3d& Q2)
	{
		constexpr double Epsilon = 1e-18;
		const FVector3d D1 = Q1 - P1;
		const FVector3d D2 = Q2 - P2;
		const FVector3d R = P1 - P2;
		const double A = Dot(D1, D1);
		const double E = Dot(D2, D2);
		const double F = Dot(D2, R);

		double S = 0.0;
		double T = 0.0;

		if (A <= Epsilon && E <= Epsilon)
		{
			return (P1 - P2).SizeSquared();
		}
		if (A <= Epsilon)
		{
			T = FMath::Clamp(F / E, 0.0, 1.0);
		}
		else
		{
			const double C = Dot(D1, R);
			if (E <= Epsilon)
			{
				S = FMath::Clamp(-C / A, 0.0, 1.0);
			}
			else
			{
				const double B = Dot(D1, D2);
				const double Denom = A * E - B * B;
				S = (Denom > Epsilon) ? FMath::Clamp((B * F - C * E) / Denom, 0.0, 1.0) : 0.0;
				T = (B * S + F) / E;
				if (T < 0.0)
				{
					T = 0.0;
					S = FMath::Clamp(-C / A, 0.0, 1.0);
				}
				else if (T > 1.0)
				{
					T = 1.0;
					S = FMath::Clamp((B - C) / A, 0.0, 1.0);
				}
			}
		}

		const FVector3d C1 = P1 + D1 * S;
		const FVector3d C2 = P2 + D2 * T;
		return (C1 - C2).SizeSquared();
	}

	/** Moller-Trumbore restricted to the segment P0-P1. */
	bool SegmentIntersectsTriangle(const FVector3d& P0, const FVector3d& P1, const FVector3d& A, const FVector3d& B, const FVector3d& C)
	{
		const FVector3d Dir = P1 - P0;
		const FVector3d E1 = B - A;
		const FVector3d E2 = C - A;
		const FVector3d H = Cross(Dir, E2);
		const double Det = Dot(E1, H);
		if (FMath::Abs(Det) < 1e-12)
		{
			return false; // Parallel: coplanar overlap is caught by the distance tests
		}
		const double InvDet = 1.0 / Det;
		const FVector3d S = P0 - A;
		const double U = InvDet * Dot(S, H);
		if (U < 0.0 || U > 1.0)
		{
			return false;
		}
		const FVector3d Q = Cross(S, E1);
		const double V = InvDet * Dot(Dir, Q);
		if (V < 0.0 || U + V > 1.0)
		{
			return false;
		}
		const double T = InvDet * Dot(E2, Q);
		return T >= 0.0 && T <= 1.0;
	}

	bool IsDegenerate(const FVector3d T[3])
	{
		const FVector3d N = Cross(T[1] - T[0], T[2] - T[0]);
		const double Scale = FMath::Max((T[1] - T[0]).SizeSquared(), (T[2] - T[0]).SizeSquared());
		// Area^2 negligible relative to edge length^2 squared
		return N.SizeSquared() <= Scale * Scale * 1e-20 || Scale <= 1e-24;
	}

	// ------------------------------------------------------------------
	// Spatial hash key (collisions only add candidates; exact distance is always checked)
	// ------------------------------------------------------------------
	inline uint64 HashCell(int64 X, int64 Y, int64 Z)
	{
		return (uint64(X) * 73856093ull) ^ (uint64(Y) * 19349663ull) ^ (uint64(Z) * 83492791ull);
	}

	inline int64 CellCoord(double Value, double InvCellSize)
	{
		return int64(FMath::FloorToDouble(Value * InvCellSize));
	}

	/** Unions all vertices within Tolerance of each other. */
	void WeldVertices(const FCoreMeshInput& Input, const TArray<bool>& bReferenced, double Tolerance, FUnionFind& VertexSets)
	{
		const double Tol = FMath::Max(Tolerance, 1e-6);
		const double TolSq = Tol * Tol;
		const double InvCell = 1.0 / Tol;

		TMap<uint64, TArray<int32>> Grid;
		for (int32 V = 0; V < Input.Positions.Num(); ++V)
		{
			if (!bReferenced[V])
			{
				continue;
			}
			const FVector3d& P = Input.Positions[V];
			const int64 CX = CellCoord(P.X, InvCell);
			const int64 CY = CellCoord(P.Y, InvCell);
			const int64 CZ = CellCoord(P.Z, InvCell);

			for (int64 DX = -1; DX <= 1; ++DX)
			{
				for (int64 DY = -1; DY <= 1; ++DY)
				{
					for (int64 DZ = -1; DZ <= 1; ++DZ)
					{
						if (const TArray<int32>* Cell = Grid.Find(HashCell(CX + DX, CY + DY, CZ + DZ)))
						{
							for (const int32 Other : *Cell)
							{
								if ((Input.Positions[Other] - P).SizeSquared() <= TolSq)
								{
									VertexSets.Union(V, Other);
								}
							}
						}
					}
				}
			}
			Grid.FindOrAdd(HashCell(CX, CY, CZ)).Add(V);
		}
	}

	/** Compacts arbitrary root ids into 0..N-1 in order of first appearance. */
	int32 CompactIds(const TArray<int32>& Roots, TArray<int32>& OutCompact, int32 RootSpace)
	{
		TArray<int32> RootToCompact;
		RootToCompact.Init(INDEX_NONE, RootSpace);
		OutCompact.SetNumUninitialized(Roots.Num());
		int32 Count = 0;
		for (int32 Index = 0; Index < Roots.Num(); ++Index)
		{
			int32& Slot = RootToCompact[Roots[Index]];
			if (Slot == INDEX_NONE)
			{
				Slot = Count++;
			}
			OutCompact[Index] = Slot;
		}
		return Count;
	}

	void GetTriangle(const FCoreMeshInput& Input, int32 Tri, FVector3d Out[3])
	{
		Out[0] = Input.Positions[Input.TriangleVertices[Tri * 3 + 0]];
		Out[1] = Input.Positions[Input.TriangleVertices[Tri * 3 + 1]];
		Out[2] = Input.Positions[Input.TriangleVertices[Tri * 3 + 2]];
	}
} // namespace CoreImpl

double TriangleTriangleDistanceSq(const FVector3d A[3], const FVector3d B[3])
{
	using namespace CoreImpl;

	const bool bDegA = IsDegenerate(A);
	const bool bDegB = IsDegenerate(B);

	// Intersection -> 0
	for (int32 E = 0; E < 3; ++E)
	{
		if (!bDegA && SegmentIntersectsTriangle(B[E], B[(E + 1) % 3], A[0], A[1], A[2]))
		{
			return 0.0;
		}
		if (!bDegB && SegmentIntersectsTriangle(A[E], A[(E + 1) % 3], B[0], B[1], B[2]))
		{
			return 0.0;
		}
	}

	double Best = TNumericLimits<double>::Max();

	// Vertex vs face (skipped for degenerate faces; their edges cover them below)
	for (int32 V = 0; V < 3; ++V)
	{
		if (!bDegB)
		{
			Best = FMath::Min(Best, (A[V] - ClosestPointOnTriangle(A[V], B[0], B[1], B[2])).SizeSquared());
		}
		if (!bDegA)
		{
			Best = FMath::Min(Best, (B[V] - ClosestPointOnTriangle(B[V], A[0], A[1], A[2])).SizeSquared());
		}
	}

	// Edge vs edge
	for (int32 EA = 0; EA < 3; ++EA)
	{
		for (int32 EB = 0; EB < 3; ++EB)
		{
			Best = FMath::Min(Best, SegmentSegmentDistanceSq(A[EA], A[(EA + 1) % 3], B[EB], B[(EB + 1) % 3]));
		}
	}

	return Best;
}

double ComputeBoundsDiagonal(const FCoreMeshInput& Input)
{
	CoreImpl::FAabb Box;
	for (const int32 V : Input.TriangleVertices)
	{
		Box.Add(Input.Positions[V]);
	}
	return Box.IsValid() ? FMath::Sqrt((Box.Max - Box.Min).SizeSquared()) : 0.0;
}

void ComputeSplitGroups(const FCoreMeshInput& Input, const FCoreSplitParams& Params, FCoreSplitResult& OutResult)
{
	using namespace CoreImpl;

	const int32 NumTris = Input.NumTriangles();
	const int32 NumVerts = Input.Positions.Num();

	OutResult = FCoreSplitResult();
	if (NumTris == 0)
	{
		return;
	}

	// --------------------------------------------------------------
	// 1) Connectivity islands (optionally welded). Always computed:
	//    Proximity builds on it, and it is a useful diagnostic.
	// --------------------------------------------------------------
	FUnionFind VertexSets(NumVerts);

	TArray<bool> bReferenced;
	bReferenced.Init(false, NumVerts);
	for (const int32 V : Input.TriangleVertices)
	{
		bReferenced[V] = true;
	}

	if (Params.bWeldVertices && Params.Mode != ECoreSplitMode::MaterialSlot)
	{
		WeldVertices(Input, bReferenced, Params.WeldTolerance, VertexSets);
	}

	for (int32 Tri = 0; Tri < NumTris; ++Tri)
	{
		const int32 V0 = Input.TriangleVertices[Tri * 3 + 0];
		VertexSets.Union(V0, Input.TriangleVertices[Tri * 3 + 1]);
		VertexSets.Union(V0, Input.TriangleVertices[Tri * 3 + 2]);
	}

	TArray<int32> TriRoots;
	TriRoots.SetNumUninitialized(NumTris);
	for (int32 Tri = 0; Tri < NumTris; ++Tri)
	{
		TriRoots[Tri] = VertexSets.Find(Input.TriangleVertices[Tri * 3]);
	}

	TArray<int32> TriIsland;
	const int32 NumIslands = CompactIds(TriRoots, TriIsland, NumVerts);
	OutResult.NumIslands = NumIslands;

	// --------------------------------------------------------------
	// 2) Mode-specific grouping
	// --------------------------------------------------------------
	if (Params.Mode == ECoreSplitMode::MaterialSlot)
	{
		int32 MaxMaterial = 0;
		for (const int32 M : Input.TriangleMaterials)
		{
			MaxMaterial = FMath::Max(MaxMaterial, M);
		}
		OutResult.NumGroups = CompactIds(Input.TriangleMaterials, OutResult.TriangleGroups, MaxMaterial + 1);
		return;
	}

	if (Params.Mode == ECoreSplitMode::Connectivity || NumIslands <= 1)
	{
		OutResult.TriangleGroups = TriIsland;
		OutResult.NumGroups = NumIslands;
		return;
	}

	// Proximity: merge islands whose closest triangles are within MergeDistance
	const double Dist = FMath::Max(Params.MergeDistance, 0.0);
	const double DistSq = Dist * Dist;

	TArray<FAabb> TriBounds;
	TriBounds.SetNum(NumTris);
	TArray<FAabb> IslandBounds;
	IslandBounds.SetNum(NumIslands);
	TArray<TArray<int32>> IslandTris;
	IslandTris.SetNum(NumIslands);

	for (int32 Tri = 0; Tri < NumTris; ++Tri)
	{
		FVector3d T[3];
		GetTriangle(Input, Tri, T);
		TriBounds[Tri].Add(T[0]);
		TriBounds[Tri].Add(T[1]);
		TriBounds[Tri].Add(T[2]);
		IslandBounds[TriIsland[Tri]].Add(TriBounds[Tri]);
		IslandTris[TriIsland[Tri]].Add(Tri);
	}

	FUnionFind IslandSets(NumIslands);
	TArray<int32> CandA;
	TArray<int32> CandB;

	for (int32 I = 0; I < NumIslands; ++I)
	{
		for (int32 J = I + 1; J < NumIslands; ++J)
		{
			if (!IslandBounds[I].OverlapsWithMargin(IslandBounds[J], Dist))
			{
				continue;
			}
			if (IslandSets.Find(I) == IslandSets.Find(J))
			{
				continue; // Already merged through another island
			}

			// Narrow phase: only triangles near the other island's bounds
			CandA.Reset();
			CandB.Reset();
			for (const int32 Tri : IslandTris[I])
			{
				if (TriBounds[Tri].OverlapsWithMargin(IslandBounds[J], Dist))
				{
					CandA.Add(Tri);
				}
			}
			for (const int32 Tri : IslandTris[J])
			{
				if (TriBounds[Tri].OverlapsWithMargin(IslandBounds[I], Dist))
				{
					CandB.Add(Tri);
				}
			}

			bool bClose = false;
			for (int32 IA = 0; IA < CandA.Num() && !bClose; ++IA)
			{
				const int32 TriA = CandA[IA];
				FVector3d TA[3];
				GetTriangle(Input, TriA, TA);

				for (int32 IB = 0; IB < CandB.Num(); ++IB)
				{
					const int32 TriB = CandB[IB];
					if (Params.bRespectMaterialBoundary && Input.TriangleMaterials[TriA] != Input.TriangleMaterials[TriB])
					{
						continue;
					}
					if (!TriBounds[TriA].OverlapsWithMargin(TriBounds[TriB], Dist))
					{
						continue;
					}
					FVector3d TB[3];
					GetTriangle(Input, TriB, TB);
					if (TriangleTriangleDistanceSq(TA, TB) <= DistSq)
					{
						bClose = true;
						break;
					}
				}
			}

			if (bClose)
			{
				IslandSets.Union(I, J);
			}
		}
	}

	TArray<int32> IslandRoots;
	IslandRoots.SetNumUninitialized(NumIslands);
	for (int32 I = 0; I < NumIslands; ++I)
	{
		IslandRoots[I] = IslandSets.Find(I);
	}

	// Group numbering follows triangle order, not island order
	TArray<int32> TriGroupRoots;
	TriGroupRoots.SetNumUninitialized(NumTris);
	for (int32 Tri = 0; Tri < NumTris; ++Tri)
	{
		TriGroupRoots[Tri] = IslandRoots[TriIsland[Tri]];
	}
	OutResult.NumGroups = CompactIds(TriGroupRoots, OutResult.TriangleGroups, NumIslands);
}

} // namespace MIS
