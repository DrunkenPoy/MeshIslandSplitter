// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#include "MISSplitterLibrary.h"

#include "MISActorReplacer.h"

#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"

bool UMISSplitterLibrary::AnalyzeStaticMesh(UStaticMesh* SourceMesh, const FMISSplitSettings& Settings, FMISSplitAnalysis& OutAnalysis)
{
	return FMISMeshSplitter::Analyze(SourceMesh, Settings, OutAnalysis);
}

bool UMISSplitterLibrary::SplitStaticMesh(UStaticMesh* SourceMesh, const FMISSplitSettings& Settings, TArray<FMISSplitPart>& OutParts)
{
	return FMISMeshSplitter::Split(SourceMesh, Settings, OutParts);
}

int32 UMISSplitterLibrary::SplitStaticMeshes(const TArray<UStaticMesh*>& SourceMeshes, const FMISSplitSettings& Settings, TArray<FMISSplitPart>& OutParts)
{
	OutParts.Reset();
	int32 NumSplit = 0;
	for (UStaticMesh* Mesh : SourceMeshes)
	{
		TArray<FMISSplitPart> Parts;
		if (FMISMeshSplitter::Split(Mesh, Settings, Parts))
		{
			++NumSplit;
			OutParts.Append(Parts);
		}
	}
	return NumSplit;
}

TArray<AStaticMeshActor*> UMISSplitterLibrary::GetLevelActorsUsingMesh(UStaticMesh* SourceMesh)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	return FMISActorReplacer::FindActorsUsingMesh(World, SourceMesh);
}

int32 UMISSplitterLibrary::ReplaceActorsWithParts(UStaticMesh* SourceMesh, const TArray<FMISSplitPart>& Parts, const TArray<AStaticMeshActor*>& Actors,
	const FMISReplaceSettings& Settings, TArray<AActor*>& OutNewActors)
{
	OutNewActors.Reset();
	return FMISActorReplacer::ReplaceActors(SourceMesh, Parts, Actors, Settings, OutNewActors);
}
