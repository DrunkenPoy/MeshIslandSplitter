// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#include "MISEditorActions.h"
#include "MISActorReplacer.h"
#include "MISMeshSplitter.h"
#include "MISSplitDialog.h"
#include "MISSplitOptions.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IContentBrowserSingleton.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "MISEditorActions"

namespace MISEditorActionsImpl
{
	/** 알림 본문에 나열할 최대 줄 수. 나머지는 "...외 N개"로 줄인다. */
	constexpr int32 MaxDetailLines = 8;

	FText JoinLines(const TArray<FText>& Lines)
	{
		TArray<FText> Shown;
		for (int32 Index = 0; Index < Lines.Num() && Index < MaxDetailLines; ++Index)
		{
			Shown.Add(Lines[Index]);
		}
		if (Lines.Num() > MaxDetailLines)
		{
			Shown.Add(FText::Format(LOCTEXT("MoreLines", "...and {0} more (see Output Log)"), FText::AsNumber(Lines.Num() - MaxDetailLines)));
		}
		return FText::Join(FText::FromString(TEXT("\n")), Shown);
	}

	void Notify(const FText& Title, const TArray<FText>& Lines, bool bSuccess)
	{
		FNotificationInfo Info(Title);
		Info.SubText = JoinLines(Lines);
		Info.ExpireDuration = bSuccess ? 5.0f : 8.0f;
		Info.bFireAndForget = true;
		Info.bUseLargeFont = false;

		const TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
		if (Item.IsValid())
		{
			Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}

	UWorld* GetEditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}
} // namespace MISEditorActionsImpl

void MISEditorActions::AnalyzeMeshes(TConstArrayView<UStaticMesh*> Meshes, const FMISSplitSettings& Settings)
{
	using namespace MISEditorActionsImpl;

	if (Meshes.Num() == 0)
	{
		return;
	}

	TArray<FText> Lines;
	int32 NumFailed = 0;
	for (UStaticMesh* Mesh : Meshes)
	{
		FMISSplitAnalysis Analysis;
		FText Error;
		if (!FMISMeshSplitter::Analyze(Mesh, Settings, Analysis, &Error))
		{
			++NumFailed;
			Lines.Add(Error);
			continue;
		}

		FString Counts;
		for (int32 Index = 0; Index < Analysis.PartTriangleCounts.Num(); ++Index)
		{
			Counts += FString::Printf(TEXT("%s%d"), Index ? TEXT(", ") : TEXT(""), Analysis.PartTriangleCounts[Index]);
		}
		UE_LOG(LogMISSplitter, Display, TEXT("  part triangle counts: [%s]"), *Counts);

		Lines.Add(FText::Format(LOCTEXT("AnalyzeLine", "{0}: {1} island(s) -> {2} part(s)"),
			FText::FromString(Mesh->GetName()), FText::AsNumber(Analysis.IslandCount), FText::AsNumber(Analysis.PartCount)));
	}

	Notify(LOCTEXT("AnalyzeTitle", "Mesh Island Splitter: Analysis"), Lines, NumFailed == 0);
}

void MISEditorActions::SplitMeshes(TConstArrayView<UStaticMesh*> Meshes, const FMISSplitSettings& Settings, const FReplaceRequest* Replace)
{
	using namespace MISEditorActionsImpl;

	if (Meshes.Num() == 0)
	{
		return;
	}

	UWorld* EditorWorld = Replace ? GetEditorWorld() : nullptr;

	TArray<UObject*> Created;
	TArray<AActor*> NewActors;
	TArray<FText> Lines;
	int32 NumSplit = 0;
	int32 NumSkipped = 0;
	int32 NumFailed = 0;
	int32 NumReplaced = 0;
	int32 NumActorsNotReplaceable = 0;
	bool bCancelled = false;

	{
		FScopedSlowTask SlowTask(float(Meshes.Num()), LOCTEXT("SplittingMeshes", "Splitting static meshes..."));
		SlowTask.MakeDialog(/*bShowCancelButton*/ Meshes.Num() > 1);

		for (UStaticMesh* Mesh : Meshes)
		{
			if (SlowTask.ShouldCancel())
			{
				bCancelled = true;
				break;
			}
			SlowTask.EnterProgressFrame(1.0f, FText::FromString(Mesh ? Mesh->GetName() : FString()));

			TArray<FMISSplitPart> Parts;
			FText Error;
			bool bNothingToSplit = false;
			if (!FMISMeshSplitter::Split(Mesh, Settings, Parts, &Error, &bNothingToSplit))
			{
				++(bNothingToSplit ? NumSkipped : NumFailed);
				Lines.Add(Error);
				continue;
			}

			++NumSplit;
			for (const FMISSplitPart& Part : Parts)
			{
				Created.Add(Part.Mesh);
			}

			FText Line = FText::Format(LOCTEXT("SplitLine", "{0}: {1} part(s)"), FText::FromString(Mesh->GetName()), FText::AsNumber(Parts.Num()));

			if (Replace)
			{
				int32 NumNotReplaceable = 0;
				const TArray<AStaticMeshActor*> Actors = Replace->RestrictToActors.Num() > 0
					? FMISActorReplacer::FilterActorsUsingMesh(Replace->RestrictToActors, Mesh, &NumNotReplaceable)
					: FMISActorReplacer::FindActorsUsingMesh(EditorWorld, Mesh, &NumNotReplaceable);
				const int32 MeshReplaced = FMISActorReplacer::ReplaceActors(Mesh, Parts, Actors, Replace->Settings, NewActors);
				NumReplaced += MeshReplaced;
				NumActorsNotReplaceable += NumNotReplaceable;
				Line = FText::Format(LOCTEXT("SplitReplaceLine", "{0}, {1} actor(s) replaced"), Line, FText::AsNumber(MeshReplaced));
			}
			Lines.Add(Line);
		}
	}

	if (Created.Num() > 0)
	{
		FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowser.Get().SyncBrowserToAssets(Created);
	}

	if (NewActors.Num() > 0 && GEditor)
	{
		if (UEditorActorSubsystem* ActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
		{
			ActorSubsystem->SetSelectedLevelActors(NewActors);
		}
	}

	if (NumSkipped > 0)
	{
		Lines.Add(FText::Format(LOCTEXT("SkippedLine", "{0} mesh(es) skipped: nothing to split with the current settings."), FText::AsNumber(NumSkipped)));
	}
	if (NumActorsNotReplaceable > 0)
	{
		Lines.Add(FText::Format(LOCTEXT("NotReplaceableLine", "{0} actor(s) not replaced: only plain Static Mesh Actors are supported (not Blueprints or subclasses)."),
			FText::AsNumber(NumActorsNotReplaceable)));
	}
	if (bCancelled)
	{
		Lines.Add(LOCTEXT("CancelledLine", "Cancelled; remaining meshes were not processed."));
	}

	FText Title = FText::Format(LOCTEXT("SplitTitle", "Mesh Island Splitter: created {0} asset(s) from {1} of {2} mesh(es)"),
		FText::AsNumber(Created.Num()), FText::AsNumber(NumSplit), FText::AsNumber(Meshes.Num()));
	if (Replace)
	{
		Title = FText::Format(LOCTEXT("SplitReplaceTitle", "{0}, replaced {1} actor(s)"), Title, FText::AsNumber(NumReplaced));
	}
	UE_LOG(LogMISSplitter, Log, TEXT("%s"), *Title.ToString());
	Notify(Title, Lines, NumFailed == 0 && NumSplit > 0 && !bCancelled);
}

TArray<UStaticMesh*> MISEditorActions::LoadStaticMeshesInPaths(TConstArrayView<FString> PackagePaths)
{
	FARFilter Filter;
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	for (const FString& Path : PackagePaths)
	{
		Filter.PackagePaths.Add(FName(*Path));
	}

	TArray<FAssetData> Assets;
	if (Filter.PackagePaths.Num() > 0)
	{
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssets(Filter, Assets);
	}
	Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });

	TArray<UStaticMesh*> Meshes;
	FScopedSlowTask SlowTask(float(Assets.Num()), LOCTEXT("LoadingMeshes", "Loading static meshes..."));
	SlowTask.MakeDialogDelayed(0.5f);
	for (const FAssetData& Asset : Assets)
	{
		SlowTask.EnterProgressFrame(1.0f);
		if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset()))
		{
			Meshes.Add(Mesh);
		}
	}
	return Meshes;
}

TArray<UStaticMesh*> MISEditorActions::GetStaticMeshesOfActors(TConstArrayView<AActor*> Actors)
{
	TArray<UStaticMesh*> Meshes;
	for (AActor* Actor : Actors)
	{
		if (const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor))
		{
			if (UStaticMesh* Mesh = MeshActor->GetStaticMeshComponent()->GetStaticMesh();
				Mesh && MeshActor->GetClass() == AStaticMeshActor::StaticClass())
			{
				Meshes.AddUnique(Mesh);
			}
		}
	}
	return Meshes;
}

void MISEditorActions::AnalyzeWithSavedOptions(TConstArrayView<UStaticMesh*> Meshes)
{
	AnalyzeMeshes(Meshes, UMISSplitOptions::Get().Split);
}

void MISEditorActions::SplitWithSavedOptions(TConstArrayView<UStaticMesh*> Meshes)
{
	const UMISSplitOptions& Options = UMISSplitOptions::Get();
	FReplaceRequest Replace;
	Replace.Settings = Options.Replace;
	SplitMeshes(Meshes, Options.Split, Options.bReplaceActorsInLevel ? &Replace : nullptr);
}

void MISEditorActions::OpenDialogForAssets(TConstArrayView<UStaticMesh*> Meshes)
{
	if (Meshes.Num() == 0)
	{
		return;
	}
	const FText Description = FText::Format(LOCTEXT("AssetsDescription", "{0} Static Mesh(es) selected. New part assets are created; the originals are not modified."),
		FText::AsNumber(Meshes.Num()));
	if (MISSplitDialog::Show(Description, Meshes, MISSplitDialog::EScope::Assets))
	{
		SplitWithSavedOptions(Meshes);
	}
}

void MISEditorActions::OpenDialogForFolders(TConstArrayView<FString> PackagePaths)
{
	using namespace MISEditorActionsImpl;

	const TArray<UStaticMesh*> Meshes = LoadStaticMeshesInPaths(PackagePaths);
	if (Meshes.Num() == 0)
	{
		Notify(LOCTEXT("NoMeshesInFolder", "Mesh Island Splitter: no Static Meshes found in the selected folder(s)."), {}, false);
		return;
	}

	const FText Description = FText::Format(LOCTEXT("FoldersDescription", "Batch: {0} Static Mesh(es) found in {1} (including subfolders). Meshes with nothing to split are skipped."),
		FText::AsNumber(Meshes.Num()), FText::FromString(FString::Join(PackagePaths, TEXT(", "))));
	if (MISSplitDialog::Show(Description, Meshes, MISSplitDialog::EScope::Assets))
	{
		SplitWithSavedOptions(Meshes);
	}
}

void MISEditorActions::OpenDialogForActors(TConstArrayView<AActor*> Actors)
{
	using namespace MISEditorActionsImpl;

	const TArray<UStaticMesh*> Meshes = GetStaticMeshesOfActors(Actors);
	if (Meshes.Num() == 0)
	{
		Notify(LOCTEXT("NoActors", "Mesh Island Splitter: select one or more Static Mesh Actors (Blueprints and subclasses are not supported)."), {}, false);
		return;
	}

	const FText Description = FText::Format(LOCTEXT("ActorsDescription", "{0} selected actor(s) using {1} Static Mesh(es) will be replaced by part actors. Other actors using the same meshes are left unchanged."),
		FText::AsNumber(Actors.Num()), FText::AsNumber(Meshes.Num()));
	if (MISSplitDialog::Show(Description, Meshes, MISSplitDialog::EScope::SelectedActors))
	{
		const UMISSplitOptions& Options = UMISSplitOptions::Get();
		FReplaceRequest Replace;
		Replace.Settings = Options.Replace;
		Replace.RestrictToActors = TArray<AActor*>(Actors);
		SplitMeshes(Meshes, Options.Split, &Replace);
	}
}

#undef LOCTEXT_NAMESPACE
