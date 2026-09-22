// Copyright (c) 2026 SulPoi
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class MeshIslandSplitter : ModuleRules
{
	public MeshIslandSplitter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UnrealEd",
			"ToolMenus",
			"ContentBrowser",
			"AssetTools",
			"AssetRegistry",

			// Mesh processing. The split algorithm itself is self-contained (MISSplitCore)
			// and does not depend on GeometryProcessing.
			"MeshDescription",
			"StaticMeshDescription",
		});
	}
}
