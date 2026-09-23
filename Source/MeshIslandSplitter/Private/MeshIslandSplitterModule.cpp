// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#include "MeshIslandSplitterModule.h"
#include "MISEditorActions.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserMenuContexts.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "ToolMenus.h"

DEFINE_LOG_CATEGORY(LogMeshIslandSplitter);

#define LOCTEXT_NAMESPACE "FMeshIslandSplitterModule"

namespace MISMenu
{
	TArray<UStaticMesh*> GetSelectedStaticMeshes(const FToolMenuContext& MenuContext)
	{
		TArray<UStaticMesh*> Meshes;
		if (const UContentBrowserAssetContextMenuContext* Context = MenuContext.FindContext<UContentBrowserAssetContextMenuContext>())
		{
			for (const FAssetData& Asset : Context->SelectedAssets)
			{
				if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset()))
				{
					Meshes.Add(Mesh);
				}
			}
		}
		return Meshes;
	}

	TArray<AActor*> GetSelectedLevelActors()
	{
		TArray<AActor*> Actors;
		if (GEditor)
		{
			GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(Actors);
		}
		return Actors;
	}

	void FillSubMenu(UToolMenu* SubMenu)
	{
		FToolMenuSection& Section = SubMenu->AddSection("MeshIslandSplitter", LOCTEXT("SectionLabel", "Mesh Island Splitter"));

		Section.AddMenuEntry(
			"MIS_SplitDialog",
			LOCTEXT("SplitDialogLabel", "Split into Islands..."),
			LOCTEXT("SplitDialogTooltip", "Open the options dialog (split mode, weld tolerance, pivot, naming, level actor replacement), then split the selected meshes. The originals are not modified."),
			FSlateIcon(),
			FToolUIAction(FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& Context)
			{
				MISEditorActions::OpenDialogForAssets(GetSelectedStaticMeshes(Context));
			})));

		Section.AddMenuEntry(
			"MIS_Split",
			LOCTEXT("SplitLabel", "Split with Last Options"),
			LOCTEXT("SplitTooltip", "Split the selected meshes immediately using the options last used in the dialog."),
			FSlateIcon(),
			FToolUIAction(FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& Context)
			{
				MISEditorActions::SplitWithSavedOptions(GetSelectedStaticMeshes(Context));
			})));

		Section.AddMenuEntry(
			"MIS_Analyze",
			LOCTEXT("AnalyzeLabel", "Analyze Islands"),
			LOCTEXT("AnalyzeTooltip", "Dry run with the last used options: report how many islands and parts each selected Static Mesh would produce. Nothing is created."),
			FSlateIcon(),
			FToolUIAction(FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& Context)
			{
				MISEditorActions::AnalyzeWithSavedOptions(GetSelectedStaticMeshes(Context));
			})));
	}
} // namespace MISMenu

void FMeshIslandSplitterModule::StartupModule()
{
	UE_LOG(LogMeshIslandSplitter, Log, TEXT("MeshIslandSplitter module started."));
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMeshIslandSplitterModule::RegisterMenus));
}

void FMeshIslandSplitterModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FMeshIslandSplitterModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	// 스태틱 메시 애셋 우클릭 메뉴 -> "Asset Actions" 섹션에 서브메뉴 추가
	{
		UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UStaticMesh::StaticClass());
		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
		Section.AddSubMenu(
			"MeshIslandSplitter",
			LOCTEXT("SubMenuLabel", "Mesh Island Splitter"),
			LOCTEXT("SubMenuTooltip", "Split physically disconnected islands into separate Static Meshes."),
			FNewToolMenuDelegate::CreateStatic(&MISMenu::FillSubMenu),
			/*bInOpenSubMenuOnClick*/ false,
			FSlateIcon());
	}

	// 폴더 우클릭 메뉴 -> 폴더(하위 폴더 포함)의 모든 스태틱 메시 일괄 처리
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.FolderContextMenu");
		FToolMenuSection& Section = Menu->FindOrAddSection("PathContextBulkOperations");
		Section.AddMenuEntry(
			"MIS_SplitFolder",
			LOCTEXT("SplitFolderLabel", "Split Static Meshes in Folder..."),
			LOCTEXT("SplitFolderTooltip", "Batch: split every Static Mesh in the selected folder(s), including subfolders, with the Mesh Island Splitter options dialog."),
			FSlateIcon(),
			FToolUIAction(FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& Context)
			{
				if (const UContentBrowserFolderContext* FolderContext = Context.FindContext<UContentBrowserFolderContext>())
				{
					MISEditorActions::OpenDialogForFolders(FolderContext->GetSelectedPackagePaths());
				}
			})));
	}

	// 레벨 뷰포트/아웃라이너 액터 우클릭 메뉴 -> 선택한 스태틱 메시 액터를 분할 파트로 치환
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContextMenu");
		FToolMenuSection& Section = Menu->FindOrAddSection("ActorTypeTools");
		Section.AddDynamicEntry("MIS_SplitActors", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{
			if (MISEditorActions::GetStaticMeshesOfActors(MISMenu::GetSelectedLevelActors()).Num() == 0)
			{
				return;
			}
			InSection.AddMenuEntry(
				"MIS_SplitActors",
				LOCTEXT("SplitActorsLabel", "Split and Replace with Islands..."),
				LOCTEXT("SplitActorsTooltip", "Split the Static Meshes used by the selected actors and replace those actors with one actor per part (undoable)."),
				FSlateIcon(),
				FToolUIAction(FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext&)
				{
					MISEditorActions::OpenDialogForActors(MISMenu::GetSelectedLevelActors());
				})));
		}));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMeshIslandSplitterModule, MeshIslandSplitter)
