// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#include "MeshIslandSplitterModule.h"

DEFINE_LOG_CATEGORY(LogMeshIslandSplitter);

#define LOCTEXT_NAMESPACE "FMeshIslandSplitterModule"

void FMeshIslandSplitterModule::StartupModule()
{
	UE_LOG(LogMeshIslandSplitter, Log, TEXT("MeshIslandSplitter module started."));
	// TODO(3단계): UToolMenus를 통해 UStaticMesh용 콘텐츠 브라우저 컨텍스트 메뉴 등록.
}

void FMeshIslandSplitterModule::ShutdownModule()
{
	// TODO(3단계): 메뉴 등록 해제 (UToolMenus::UnregisterOwner(this)).
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMeshIslandSplitterModule, MeshIslandSplitter)
