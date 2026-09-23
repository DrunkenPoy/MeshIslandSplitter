// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "Modules/ModuleManager.h"

MESHISLANDSPLITTER_API DECLARE_LOG_CATEGORY_EXTERN(LogMeshIslandSplitter, Log, All);

class FMeshIslandSplitterModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FMeshIslandSplitterModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FMeshIslandSplitterModule>("MeshIslandSplitter");
	}
};
