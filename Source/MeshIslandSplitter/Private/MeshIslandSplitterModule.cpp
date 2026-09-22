// Copyright (c) 2026 SulPoi. All Rights Reserved.

#include "MeshIslandSplitterModule.h"

DEFINE_LOG_CATEGORY(LogMeshIslandSplitter);

#define LOCTEXT_NAMESPACE "FMeshIslandSplitterModule"

void FMeshIslandSplitterModule::StartupModule()
{
	UE_LOG(LogMeshIslandSplitter, Log, TEXT("MeshIslandSplitter module started."));
	// TODO(step 3): Register Content Browser context menu for UStaticMesh via UToolMenus.
}

void FMeshIslandSplitterModule::ShutdownModule()
{
	// TODO(step 3): Unregister menus (UToolMenus::UnregisterOwner(this)).
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMeshIslandSplitterModule, MeshIslandSplitter)
