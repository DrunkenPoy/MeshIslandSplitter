// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT

#include "MISMeshSplitter.h"
#include "MISSplitCore.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/StaticMesh.h"
#include "IAssetTools.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogMISSplitter);

#define LOCTEXT_NAMESPACE "MISMeshSplitter"

namespace MISSplitterImpl
{
	constexpr int32 SourceLOD = 0;

	struct FExtractedMesh
	{
		MIS::FCoreMeshInput Core;
		TArray<FTriangleID> TriangleIds;	// Core triangle index -> source triangle id
	};

	void SetError(FText* OutError, const FText& Message)
	{
		if (OutError)
		{
			*OutError = Message;
		}
		UE_LOG(LogMISSplitter, Warning, TEXT("%s"), *Message.ToString());
	}

	const FMeshDescription* GetSourceDescription(const UStaticMesh* Source, FText* OutError)
	{
		if (!Source)
		{
			SetError(OutError, LOCTEXT("NoSource", "No source mesh."));
			return nullptr;
		}
		if (Source->GetNumSourceModels() <= SourceLOD)
		{
			SetError(OutError, FText::Format(LOCTEXT("NoSourceModel", "{0} has no source model for LOD0."), FText::FromString(Source->GetName())));
			return nullptr;
		}
		const FMeshDescription* Description = Source->GetMeshDescription(SourceLOD);
		if (!Description || Description->Triangles().Num() == 0)
		{
			SetError(OutError, FText::Format(LOCTEXT("NoDescription", "{0} has no mesh description (or no triangles) for LOD0."), FText::FromString(Source->GetName())));
			return nullptr;
		}
		return Description;
	}

	void Extract(const FMeshDescription& Description, FExtractedMesh& Out)
	{
		FStaticMeshConstAttributes Attributes(Description);
		const auto Positions = Attributes.GetVertexPositions();

		TArray<int32> VertexToDense;
		VertexToDense.Init(INDEX_NONE, Description.Vertices().GetArraySize());
		Out.Core.Positions.Reserve(Description.Vertices().Num());
		for (const FVertexID VertexId : Description.Vertices().GetElementIDs())
		{
			VertexToDense[VertexId.GetValue()] = Out.Core.Positions.Add(FVector3d(Positions[VertexId]));
		}

		const int32 NumTriangles = Description.Triangles().Num();
		Out.Core.TriangleVertices.Reserve(NumTriangles * 3);
		Out.Core.TriangleMaterials.Reserve(NumTriangles);
		Out.TriangleIds.Reserve(NumTriangles);

		for (const FTriangleID TriangleId : Description.Triangles().GetElementIDs())
		{
			const TArrayView<const FVertexInstanceID> Instances = Description.GetTriangleVertexInstances(TriangleId);
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const FVertexID VertexId = Description.GetVertexInstanceVertex(Instances[Corner]);
				Out.Core.TriangleVertices.Add(VertexToDense[VertexId.GetValue()]);
			}
			Out.Core.TriangleMaterials.Add(Description.GetTrianglePolygonGroup(TriangleId).GetValue());
			Out.TriangleIds.Add(TriangleId);
		}
	}

	MIS::FCoreSplitParams ResolveParams(const FMISSplitSettings& Settings, const MIS::FCoreMeshInput& Core)
	{
		MIS::FCoreSplitParams Params;
		switch (Settings.SplitMode)
		{
		case EMISSplitMode::Connectivity: Params.Mode = MIS::ECoreSplitMode::Connectivity; break;
		case EMISSplitMode::Proximity:    Params.Mode = MIS::ECoreSplitMode::Proximity; break;
		case EMISSplitMode::MaterialSlot: Params.Mode = MIS::ECoreSplitMode::MaterialSlot; break;
		}
		Params.bWeldVertices = Settings.bWeldVertices;
		Params.WeldTolerance = FMath::Max(0.0, double(Settings.WeldTolerance));
		Params.bRespectMaterialBoundary = Settings.bRespectMaterialBoundary;
		Params.MergeDistance = (Settings.DistanceUnit == EMISDistanceUnit::RelativeToBounds)
			? MIS::ComputeBoundsDiagonal(Core) * FMath::Max(0.0, double(Settings.MergeDistancePercent)) * 0.01
			: FMath::Max(0.0, double(Settings.MergeDistance));
		return Params;
	}

	FVector3d ComputePivot(const MIS::FCoreMeshInput& Core, const TArray<int32>& CoreTriangles, EMISPivotMode Mode)
	{
		if (Mode == EMISPivotMode::KeepOriginal || CoreTriangles.Num() == 0)
		{
			return FVector3d::ZeroVector;
		}

		FVector3d Min(TNumericLimits<double>::Max());
		FVector3d Max(-TNumericLimits<double>::Max());
		for (const int32 Tri : CoreTriangles)
		{
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const FVector3d& P = Core.Positions[Core.TriangleVertices[Tri * 3 + Corner]];
				Min = FVector3d::Min(Min, P);
				Max = FVector3d::Max(Max, P);
			}
		}

		const FVector3d Center = (Min + Max) * 0.5;
		return (Mode == EMISPivotMode::BoundsBottomCenter) ? FVector3d(Center.X, Center.Y, Min.Z) : Center;
	}

	/**
	 * Copies the given triangles into a fresh mesh description, keeping every per-instance attribute
	 * (normals, tangents, binormal signs, colors, all UV channels), edge hardness and material slot names.
	 * Output polygon groups are created in first-use order; OutSourceGroups lists the matching source groups.
	 */
	void BuildPartDescription(
		const FMeshDescription& Src,
		const TArray<FTriangleID>& Triangles,
		const FVector3f& PivotOffset,
		FMeshDescription& Dst,
		TArray<FPolygonGroupID>& OutSourceGroups)
	{
		FStaticMeshAttributes DstAttributes(Dst);
		DstAttributes.Register();

		FStaticMeshConstAttributes SrcAttributes(Src);
		const auto SrcPositions = SrcAttributes.GetVertexPositions();
		const auto SrcNormals = SrcAttributes.GetVertexInstanceNormals();
		const auto SrcTangents = SrcAttributes.GetVertexInstanceTangents();
		const auto SrcBinormalSigns = SrcAttributes.GetVertexInstanceBinormalSigns();
		const auto SrcColors = SrcAttributes.GetVertexInstanceColors();
		const auto SrcUVs = SrcAttributes.GetVertexInstanceUVs();
		const auto SrcEdgeHardness = SrcAttributes.GetEdgeHardnesses();
		const auto SrcSlotNames = SrcAttributes.GetPolygonGroupMaterialSlotNames();

		auto DstPositions = DstAttributes.GetVertexPositions();
		auto DstNormals = DstAttributes.GetVertexInstanceNormals();
		auto DstTangents = DstAttributes.GetVertexInstanceTangents();
		auto DstBinormalSigns = DstAttributes.GetVertexInstanceBinormalSigns();
		auto DstColors = DstAttributes.GetVertexInstanceColors();
		auto DstUVs = DstAttributes.GetVertexInstanceUVs();
		auto DstEdgeHardness = DstAttributes.GetEdgeHardnesses();
		auto DstSlotNames = DstAttributes.GetPolygonGroupMaterialSlotNames();

		const int32 NumUVChannels = SrcUVs.GetNumChannels();
		Dst.SetNumUVLayers(NumUVChannels);
		DstUVs.SetNumChannels(NumUVChannels);

		TArray<int32> VertexMap;
		VertexMap.Init(INDEX_NONE, Src.Vertices().GetArraySize());
		TArray<int32> InstanceMap;
		InstanceMap.Init(INDEX_NONE, Src.VertexInstances().GetArraySize());
		TArray<int32> GroupMap;
		GroupMap.Init(INDEX_NONE, Src.PolygonGroups().GetArraySize());
		TArray<FVertexID> DstToSrcVertex;

		OutSourceGroups.Reset();

		for (const FTriangleID SrcTriangle : Triangles)
		{
			const FPolygonGroupID SrcGroup = Src.GetTrianglePolygonGroup(SrcTriangle);
			if (GroupMap[SrcGroup.GetValue()] == INDEX_NONE)
			{
				const FPolygonGroupID NewGroup = Dst.CreatePolygonGroup();
				DstSlotNames[NewGroup] = SrcSlotNames[SrcGroup];
				GroupMap[SrcGroup.GetValue()] = NewGroup.GetValue();
				OutSourceGroups.Add(SrcGroup);
			}
			const FPolygonGroupID DstGroup(GroupMap[SrcGroup.GetValue()]);

			const TArrayView<const FVertexInstanceID> SrcInstances = Src.GetTriangleVertexInstances(SrcTriangle);
			FVertexInstanceID DstInstances[3];

			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const FVertexInstanceID SrcInstance = SrcInstances[Corner];
				int32& MappedInstance = InstanceMap[SrcInstance.GetValue()];
				if (MappedInstance == INDEX_NONE)
				{
					const FVertexID SrcVertex = Src.GetVertexInstanceVertex(SrcInstance);
					int32& MappedVertex = VertexMap[SrcVertex.GetValue()];
					if (MappedVertex == INDEX_NONE)
					{
						const FVertexID NewVertex = Dst.CreateVertex();
						DstPositions[NewVertex] = SrcPositions[SrcVertex] - PivotOffset;
						MappedVertex = NewVertex.GetValue();
						if (DstToSrcVertex.Num() <= MappedVertex)
						{
							DstToSrcVertex.SetNum(MappedVertex + 1);
						}
						DstToSrcVertex[MappedVertex] = SrcVertex;
					}

					const FVertexInstanceID NewInstance = Dst.CreateVertexInstance(FVertexID(MappedVertex));
					DstNormals[NewInstance] = SrcNormals[SrcInstance];
					DstTangents[NewInstance] = SrcTangents[SrcInstance];
					DstBinormalSigns[NewInstance] = SrcBinormalSigns[SrcInstance];
					DstColors[NewInstance] = SrcColors[SrcInstance];
					for (int32 Channel = 0; Channel < NumUVChannels; ++Channel)
					{
						DstUVs.Set(NewInstance, Channel, SrcUVs.Get(SrcInstance, Channel));
					}
					MappedInstance = NewInstance.GetValue();
				}
				DstInstances[Corner] = FVertexInstanceID(MappedInstance);
			}

			Dst.CreateTriangle(DstGroup, TArrayView<const FVertexInstanceID>(DstInstances, 3));
		}

		// Edges are created implicitly by CreateTriangle; copy hardness from the matching source edge
		for (const FEdgeID DstEdge : Dst.Edges().GetElementIDs())
		{
			const FVertexID SrcV0 = DstToSrcVertex[Dst.GetEdgeVertex(DstEdge, 0).GetValue()];
			const FVertexID SrcV1 = DstToSrcVertex[Dst.GetEdgeVertex(DstEdge, 1).GetValue()];
			const FEdgeID SrcEdge = Src.GetVertexPairEdge(SrcV0, SrcV1);
			if (SrcEdge.GetValue() != INDEX_NONE)
			{
				DstEdgeHardness[DstEdge] = SrcEdgeHardness[SrcEdge];
			}
		}
	}

	/** Finds the source material for a polygon group: slot name first, then index as a fallback. */
	FStaticMaterial FindSourceMaterial(const UStaticMesh* Source, FName SlotName, FPolygonGroupID SourceGroup)
	{
		const TArray<FStaticMaterial>& Materials = Source->GetStaticMaterials();
		for (const FStaticMaterial& Material : Materials)
		{
			if (Material.ImportedMaterialSlotName == SlotName)
			{
				return Material;
			}
		}
		for (const FStaticMaterial& Material : Materials)
		{
			if (Material.MaterialSlotName == SlotName)
			{
				return Material;
			}
		}
		if (Materials.IsValidIndex(SourceGroup.GetValue()))
		{
			return Materials[SourceGroup.GetValue()];
		}
		return FStaticMaterial(nullptr, SlotName, SlotName);
	}

	UStaticMesh* CreatePartAsset(
		const UStaticMesh* Source,
		const FString& BasePackageName,
		FMeshDescription&& PartDescription,
		const TArray<FPolygonGroupID>& SourceGroups)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		FString PackageName;
		FString AssetName;
		AssetTools.CreateUniqueAssetName(BasePackageName, FString(), PackageName, AssetName);

		UPackage* Package = CreatePackage(*PackageName);
		UStaticMesh* NewMesh = NewObject<UStaticMesh>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);

		// Materials in the same order as the polygon groups, so section N -> material N
		FStaticMeshConstAttributes PartAttributes(PartDescription);
		const auto SlotNames = PartAttributes.GetPolygonGroupMaterialSlotNames();
		TArray<FStaticMaterial> PartMaterials;
		int32 GroupIndex = 0;
		for (const FPolygonGroupID PartGroup : PartDescription.PolygonGroups().GetElementIDs())
		{
			PartMaterials.Add(FindSourceMaterial(Source, SlotNames[PartGroup], SourceGroups[GroupIndex++]));
		}
		NewMesh->SetStaticMaterials(PartMaterials);

		FStaticMeshSourceModel& SourceModel = NewMesh->AddSourceModel();
		SourceModel.BuildSettings = Source->GetSourceModel(SourceLOD).BuildSettings;

		FMeshDescription* NewDescription = NewMesh->CreateMeshDescription(SourceLOD);
		*NewDescription = MoveTemp(PartDescription);

		UStaticMesh::FCommitMeshDescriptionParams CommitParams;
		CommitParams.bMarkPackageDirty = false;
		NewMesh->CommitMeshDescription(SourceLOD, CommitParams);

		NewMesh->GetNaniteSettings() = Source->GetNaniteSettings();
		NewMesh->SetLightMapCoordinateIndex(Source->GetLightMapCoordinateIndex());
		NewMesh->SetLightMapResolution(Source->GetLightMapResolution());

		NewMesh->PostEditChange();	// Builds render data
		FAssetRegistryModule::AssetCreated(NewMesh);
		NewMesh->MarkPackageDirty();
		return NewMesh;
	}

	bool RunCore(const UStaticMesh* Source, const FMISSplitSettings& Settings, FExtractedMesh& OutMesh,
		MIS::FCoreSplitParams& OutParams, MIS::FCoreSplitResult& OutResult, FText* OutError)
	{
		const FMeshDescription* Description = GetSourceDescription(Source, OutError);
		if (!Description)
		{
			return false;
		}
		Extract(*Description, OutMesh);
		OutParams = ResolveParams(Settings, OutMesh.Core);
		MIS::ComputeSplitGroups(OutMesh.Core, OutParams, OutResult);
		return true;
	}
} // namespace MISSplitterImpl

bool FMISMeshSplitter::Analyze(const UStaticMesh* Source, const FMISSplitSettings& Settings, FMISSplitAnalysis& OutAnalysis, FText* OutError)
{
	using namespace MISSplitterImpl;

	OutAnalysis = FMISSplitAnalysis();

	FExtractedMesh Mesh;
	MIS::FCoreSplitParams Params;
	MIS::FCoreSplitResult Result;
	if (!RunCore(Source, Settings, Mesh, Params, Result, OutError))
	{
		return false;
	}

	OutAnalysis.TriangleCount = Mesh.Core.NumTriangles();
	OutAnalysis.IslandCount = Result.NumIslands;
	OutAnalysis.PartCount = Result.NumGroups;
	OutAnalysis.ResolvedMergeDistance = float(Params.MergeDistance);
	OutAnalysis.PartTriangleCounts.Init(0, Result.NumGroups);
	for (const int32 Group : Result.TriangleGroups)
	{
		++OutAnalysis.PartTriangleCounts[Group];
	}

	UE_LOG(LogMISSplitter, Log, TEXT("Analyze %s: %d triangles, %d islands -> %d parts (merge distance %.3f cm)"),
		*Source->GetName(), OutAnalysis.TriangleCount, OutAnalysis.IslandCount, OutAnalysis.PartCount, OutAnalysis.ResolvedMergeDistance);
	return true;
}

bool FMISMeshSplitter::Split(UStaticMesh* Source, const FMISSplitSettings& Settings, TArray<FMISSplitPart>& OutParts, FText* OutError)
{
	using namespace MISSplitterImpl;

	OutParts.Reset();

	FExtractedMesh Mesh;
	MIS::FCoreSplitParams Params;
	MIS::FCoreSplitResult Result;
	if (!RunCore(Source, Settings, Mesh, Params, Result, OutError))
	{
		return false;
	}

	if (Result.NumGroups < 2)
	{
		SetError(OutError, FText::Format(LOCTEXT("NothingToSplit", "{0}: nothing to split with the current settings ({1} island(s), 1 part)."),
			FText::FromString(Source->GetName()), FText::AsNumber(Result.NumIslands)));
		return false;
	}

	// Bucket triangles per group (both core indices for pivots and source ids for copying)
	TArray<TArray<int32>> GroupCoreTriangles;
	GroupCoreTriangles.SetNum(Result.NumGroups);
	TArray<TArray<FTriangleID>> GroupTriangleIds;
	GroupTriangleIds.SetNum(Result.NumGroups);
	for (int32 Tri = 0; Tri < Result.TriangleGroups.Num(); ++Tri)
	{
		const int32 Group = Result.TriangleGroups[Tri];
		GroupCoreTriangles[Group].Add(Tri);
		GroupTriangleIds[Group].Add(Mesh.TriangleIds[Tri]);
	}

	const FMeshDescription& SourceDescription = *Source->GetMeshDescription(SourceLOD);
	const FString PackagePath = FPackageName::GetLongPackagePath(Source->GetPackage()->GetName());

	FScopedSlowTask SlowTask(float(Result.NumGroups),
		FText::Format(LOCTEXT("Splitting", "Splitting {0} into {1} parts..."), FText::FromString(Source->GetName()), FText::AsNumber(Result.NumGroups)));
	SlowTask.MakeDialog(/*bShowCancelButton*/ false);

	for (int32 Group = 0; Group < Result.NumGroups; ++Group)
	{
		SlowTask.EnterProgressFrame(1.0f);

		const FVector3d Pivot = ComputePivot(Mesh.Core, GroupCoreTriangles[Group], Settings.PivotMode);

		FMeshDescription PartDescription;
		TArray<FPolygonGroupID> SourceGroups;
		BuildPartDescription(SourceDescription, GroupTriangleIds[Group], FVector3f(Pivot), PartDescription, SourceGroups);

		const FString BaseName = FString::Printf(TEXT("%s/%s%s%02d"), *PackagePath, *Source->GetName(), *Settings.PartSuffix, Group);
		UStaticMesh* PartMesh = CreatePartAsset(Source, BaseName, MoveTemp(PartDescription), SourceGroups);

		FMISSplitPart& Part = OutParts.AddDefaulted_GetRef();
		Part.Mesh = PartMesh;
		Part.PivotOffset = FVector(Pivot);
		Part.TriangleCount = GroupTriangleIds[Group].Num();
	}

	UE_LOG(LogMISSplitter, Log, TEXT("Split %s: %d islands -> %d parts in %s"),
		*Source->GetName(), Result.NumIslands, Result.NumGroups, *PackagePath);
	return true;
}

#undef LOCTEXT_NAMESPACE
