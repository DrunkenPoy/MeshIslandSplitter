// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT

#include "MISSplitterLibrary.h"

#include "Engine/StaticMesh.h"

bool UMISSplitterLibrary::AnalyzeStaticMesh(UStaticMesh* SourceMesh, const FMISSplitSettings& Settings, FMISSplitAnalysis& OutAnalysis)
{
	return FMISMeshSplitter::Analyze(SourceMesh, Settings, OutAnalysis);
}

bool UMISSplitterLibrary::SplitStaticMesh(UStaticMesh* SourceMesh, const FMISSplitSettings& Settings, TArray<FMISSplitPart>& OutParts)
{
	return FMISMeshSplitter::Split(SourceMesh, Settings, OutParts);
}
