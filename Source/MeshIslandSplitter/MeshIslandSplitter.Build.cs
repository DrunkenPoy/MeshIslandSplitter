// Copyright (c) 2026 DrunkenPoy
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

			// 메시 처리. 분할 알고리즘 자체는 독립적이며(MISSplitCore)
			// GeometryProcessing에 의존하지 않는다.
			"MeshDescription",
			"StaticMeshDescription",
		});
	}
}
