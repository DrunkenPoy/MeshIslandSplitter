// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT
//
// Minimal stand-in for the parts of UE's CoreMinimal.h used by MISSplitCore.
// Lets the split core compile and run without the engine. NOT used by UBT.

#pragma once

#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <unordered_map>
#include <algorithm>

using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint64 = uint64_t;

#define INDEX_NONE (-1)

template <typename T>
class TArray
{
	std::deque<T> Data; // deque avoids the std::vector<bool> proxy problem
public:
	int32 Num() const { return int32(Data.size()); }
	bool IsEmpty() const { return Data.empty(); }
	int32 Add(const T& Item) { Data.push_back(Item); return Num() - 1; }
	void SetNum(int32 Count) { Data.resize(Count); }
	void SetNumUninitialized(int32 Count) { Data.resize(Count); }
	void Init(const T& Value, int32 Count) { Data.assign(Count, Value); }
	void Reserve(int32) {}
	void Reset() { Data.clear(); }
	T& operator[](int32 Index) { return Data[Index]; }
	const T& operator[](int32 Index) const { return Data[Index]; }
	auto begin() { return Data.begin(); }
	auto end() { return Data.end(); }
	auto begin() const { return Data.begin(); }
	auto end() const { return Data.end(); }
};

template <typename K, typename V>
class TMap
{
	std::unordered_map<K, V> Data;
public:
	V& FindOrAdd(const K& Key) { return Data[Key]; }
	V* Find(const K& Key) { auto It = Data.find(Key); return It == Data.end() ? nullptr : &It->second; }
	const V* Find(const K& Key) const { auto It = Data.find(Key); return It == Data.end() ? nullptr : &It->second; }
	int32 Num() const { return int32(Data.size()); }
};

template <typename T>
struct TNumericLimits
{
	static constexpr T Max() { return std::numeric_limits<T>::max(); }
};

struct FVector3d
{
	double X = 0, Y = 0, Z = 0;
	FVector3d() = default;
	FVector3d(double InX, double InY, double InZ) : X(InX), Y(InY), Z(InZ) {}
	FVector3d operator+(const FVector3d& O) const { return { X + O.X, Y + O.Y, Z + O.Z }; }
	FVector3d operator-(const FVector3d& O) const { return { X - O.X, Y - O.Y, Z - O.Z }; }
	FVector3d operator*(double S) const { return { X * S, Y * S, Z * S }; }
	double SizeSquared() const { return X * X + Y * Y + Z * Z; }
	static double DotProduct(const FVector3d& A, const FVector3d& B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
	static FVector3d CrossProduct(const FVector3d& A, const FVector3d& B)
	{
		return { A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X };
	}
};

struct FMath
{
	template <typename T> static T Min(T A, T B) { return A < B ? A : B; }
	template <typename T> static T Max(T A, T B) { return A > B ? A : B; }
	template <typename T> static T Abs(T A) { return A < 0 ? -A : A; }
	template <typename T> static T Clamp(T V, T Lo, T Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }
	static double Sqrt(double V) { return std::sqrt(V); }
	static double FloorToDouble(double V) { return std::floor(V); }
};
