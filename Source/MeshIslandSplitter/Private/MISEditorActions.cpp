// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#include "MISEditorActions.h"
#include "MISMeshSplitter.h"

#include "ContentBrowserModule.h"
#include "Engine/StaticMesh.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IContentBrowserSingleton.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
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

void MISEditorActions::SplitMeshes(TConstArrayView<UStaticMesh*> Meshes, const FMISSplitSettings& Settings)
{
	using namespace MISEditorActionsImpl;

	if (Meshes.Num() == 0)
	{
		return;
	}

	TArray<UObject*> Created;
	TArray<FText> Lines;
	int32 NumSplit = 0;

	{
		FScopedSlowTask SlowTask(float(Meshes.Num()), LOCTEXT("SplittingMeshes", "Splitting static meshes..."));
		SlowTask.MakeDialog(/*bShowCancelButton*/ false);

		for (UStaticMesh* Mesh : Meshes)
		{
			SlowTask.EnterProgressFrame(1.0f, FText::FromString(Mesh ? Mesh->GetName() : FString()));

			TArray<FMISSplitPart> Parts;
			FText Error;
			if (!FMISMeshSplitter::Split(Mesh, Settings, Parts, &Error))
			{
				Lines.Add(Error);
				continue;
			}

			++NumSplit;
			for (const FMISSplitPart& Part : Parts)
			{
				Created.Add(Part.Mesh);
			}
			Lines.Add(FText::Format(LOCTEXT("SplitLine", "{0}: {1} part(s)"),
				FText::FromString(Mesh->GetName()), FText::AsNumber(Parts.Num())));
		}
	}

	if (Created.Num() > 0)
	{
		FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowser.Get().SyncBrowserToAssets(Created);
	}

	const FText Title = FText::Format(LOCTEXT("SplitTitle", "Mesh Island Splitter: created {0} asset(s) from {1} of {2} mesh(es)"),
		FText::AsNumber(Created.Num()), FText::AsNumber(NumSplit), FText::AsNumber(Meshes.Num()));
	Notify(Title, Lines, NumSplit == Meshes.Num());
}

#undef LOCTEXT_NAMESPACE
