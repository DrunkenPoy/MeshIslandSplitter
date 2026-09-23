// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#include "MeshIslandSplitterModule.h"
#include "MISEditorActions.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserMenuContexts.h"
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

	void ExecuteAnalyze(const FToolMenuContext& MenuContext)
	{
		// TODO(4단계): 옵션 UI에서 설정을 받아온다. 현재는 기본값을 사용.
		MISEditorActions::AnalyzeMeshes(GetSelectedStaticMeshes(MenuContext), FMISSplitSettings());
	}

	void ExecuteSplit(const FToolMenuContext& MenuContext)
	{
		// TODO(4단계): 옵션 UI에서 설정을 받아온다. 현재는 기본값을 사용.
		MISEditorActions::SplitMeshes(GetSelectedStaticMeshes(MenuContext), FMISSplitSettings());
	}

	void FillSubMenu(UToolMenu* SubMenu)
	{
		FToolMenuSection& Section = SubMenu->AddSection("MeshIslandSplitter", LOCTEXT("SectionLabel", "Mesh Island Splitter"));

		Section.AddMenuEntry(
			"MIS_Analyze",
			LOCTEXT("AnalyzeLabel", "Analyze Islands"),
			LOCTEXT("AnalyzeTooltip", "Dry run: report how many islands and parts each selected Static Mesh would produce. Nothing is created."),
			FSlateIcon(),
			FToolUIAction(FToolMenuExecuteAction::CreateStatic(&ExecuteAnalyze)));

		Section.AddMenuEntry(
			"MIS_Split",
			LOCTEXT("SplitLabel", "Split into Islands"),
			LOCTEXT("SplitTooltip", "Create one new Static Mesh asset per part next to each selected mesh. The originals are not modified."),
			FSlateIcon(),
			FToolUIAction(FToolMenuExecuteAction::CreateStatic(&ExecuteSplit)));
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMeshIslandSplitterModule, MeshIslandSplitter)
