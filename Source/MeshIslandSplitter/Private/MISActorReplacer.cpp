// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#include "MISActorReplacer.h"

#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ScopedTransaction.h"
#include "Subsystems/EditorActorSubsystem.h"

#define LOCTEXT_NAMESPACE "MISActorReplacer"

namespace MISActorReplacerImpl
{
	/** 파생 클래스는 복제 후 메시만 바꾸면 동작이 달라질 수 있으므로 정확히 AStaticMeshActor만 치환한다. */
	bool IsReplaceable(const AActor* Actor, const UStaticMesh* Source, bool& bOutUsesMesh)
	{
		bOutUsesMesh = false;
		const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || MeshActor->IsTemplate() || !IsValid(MeshActor))
		{
			return false;
		}
		const UStaticMeshComponent* Component = MeshActor->GetStaticMeshComponent();
		bOutUsesMesh = Component && Component->GetStaticMesh() == Source;
		return bOutUsesMesh && MeshActor->GetClass() == AStaticMeshActor::StaticClass();
	}

	/** 원본 오버라이드를 파트의 슬롯 순서에 맞게 다시 매핑한다 (슬롯 이름 기준). */
	void RemapMaterialOverrides(const UStaticMesh* Source, const TArray<TObjectPtr<UMaterialInterface>>& SourceOverrides,
		const UStaticMesh* PartMesh, UStaticMeshComponent* PartComponent)
	{
		PartComponent->EmptyOverrideMaterials();
		const TArray<FStaticMaterial>& PartMaterials = PartMesh->GetStaticMaterials();
		for (int32 PartIndex = 0; PartIndex < PartMaterials.Num(); ++PartIndex)
		{
			const int32 SourceIndex = Source->GetMaterialIndex(PartMaterials[PartIndex].MaterialSlotName);
			if (SourceOverrides.IsValidIndex(SourceIndex) && SourceOverrides[SourceIndex])
			{
				PartComponent->SetMaterial(PartIndex, SourceOverrides[SourceIndex]);
			}
		}
	}
} // namespace MISActorReplacerImpl

TArray<AStaticMeshActor*> FMISActorReplacer::FindActorsUsingMesh(UWorld* World, const UStaticMesh* Source, int32* OutNumSkipped)
{
	TArray<AActor*> Candidates;
	if (World && Source)
	{
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			Candidates.Add(*It);
		}
	}
	return FilterActorsUsingMesh(Candidates, Source, OutNumSkipped);
}

TArray<AStaticMeshActor*> FMISActorReplacer::FilterActorsUsingMesh(TConstArrayView<AActor*> Actors, const UStaticMesh* Source, int32* OutNumSkipped)
{
	TArray<AStaticMeshActor*> Result;
	int32 NumSkipped = 0;
	for (AActor* Actor : Actors)
	{
		bool bUsesMesh = false;
		if (MISActorReplacerImpl::IsReplaceable(Actor, Source, bUsesMesh))
		{
			Result.Add(CastChecked<AStaticMeshActor>(Actor));
		}
		else if (bUsesMesh)
		{
			++NumSkipped;
		}
	}
	if (OutNumSkipped)
	{
		*OutNumSkipped = NumSkipped;
	}
	return Result;
}

int32 FMISActorReplacer::ReplaceActors(
	const UStaticMesh* Source,
	TConstArrayView<FMISSplitPart> Parts,
	TConstArrayView<AStaticMeshActor*> Actors,
	const FMISReplaceSettings& Settings,
	TArray<AActor*>& OutNewActors)
{
	using namespace MISActorReplacerImpl;

	UEditorActorSubsystem* ActorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr;
	if (!Source || Parts.Num() == 0 || Actors.Num() == 0 || !ActorSubsystem)
	{
		return 0;
	}

	const FScopedTransaction Transaction(FText::Format(LOCTEXT("ReplaceTransaction", "Replace {0} with Split Parts"), FText::FromString(Source->GetName())));

	TArray<AActor*> ActorsToDestroy;
	int32 NumReplaced = 0;

	for (AStaticMeshActor* Original : Actors)
	{
		bool bUsesMesh = false;
		if (!IsReplaceable(Original, Source, bUsesMesh))
		{
			continue;
		}

		const UStaticMeshComponent* OriginalComponent = Original->GetStaticMeshComponent();
		const TArray<TObjectPtr<UMaterialInterface>> OriginalOverrides = OriginalComponent->OverrideMaterials;
		const FTransform OriginalTransform = Original->GetActorTransform();
		const FString OriginalLabel = Original->GetActorLabel();

		FName PartFolder = Original->GetFolderPath();
		if (Settings.bGroupInOutlinerFolder)
		{
			PartFolder = PartFolder.IsNone() ? FName(*OriginalLabel) : FName(*(PartFolder.ToString() / OriginalLabel));
		}

		UEditorActorSubsystem::FActorDuplicateParameters DuplicateParams;
		DuplicateParams.LevelOverride = Original->GetLevel();
		DuplicateParams.bTransact = false;	// 바깥 트랜잭션 하나로 묶는다

		AActor* FirstPart = nullptr;
		for (const FMISSplitPart& Part : Parts)
		{
			if (!Part.Mesh)
			{
				continue;
			}

			AStaticMeshActor* PartActor = Cast<AStaticMeshActor>(ActorSubsystem->DuplicateActor(Original, Original->GetWorld(), FVector::ZeroVector, DuplicateParams));
			if (!PartActor)
			{
				continue;
			}

			PartActor->Modify();
			UStaticMeshComponent* PartComponent = PartActor->GetStaticMeshComponent();
			PartComponent->Modify();
			PartComponent->SetStaticMesh(Part.Mesh);
			RemapMaterialOverrides(Source, OriginalOverrides, Part.Mesh, PartComponent);

			// 파트 메시의 원점은 원본 공간의 PivotOffset에 있다
			PartActor->SetActorTransform(FTransform(Part.PivotOffset) * OriginalTransform);
			PartActor->SetActorLabel(Part.Mesh->GetName());
			PartActor->SetFolderPath(PartFolder);

			OutNewActors.Add(PartActor);
			if (!FirstPart)
			{
				FirstPart = PartActor;
			}
		}

		if (!FirstPart)
		{
			continue;
		}
		++NumReplaced;

		if (Settings.bDeleteOriginalActors)
		{
			// 원본에 붙어 있던 자식 액터는 첫 번째 파트로 옮겨 월드 위치를 유지한다
			TArray<AActor*> Children;
			Original->GetAttachedActors(Children, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ false);
			for (AActor* Child : Children)
			{
				Child->Modify();
				Child->AttachToActor(FirstPart, FAttachmentTransformRules::KeepWorldTransform);
			}
			ActorsToDestroy.Add(Original);
		}
	}

	if (ActorsToDestroy.Num() > 0)
	{
		ActorSubsystem->DestroyActors(ActorsToDestroy);
	}
	return NumReplaced;
}

#undef LOCTEXT_NAMESPACE
