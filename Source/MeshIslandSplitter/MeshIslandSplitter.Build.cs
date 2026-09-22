// Copyright (c) 2026 SulPoi. All Rights Reserved.

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

			// --- Mesh processing (enable in step 2: splitter core) ---
			// "MeshDescription",
			// "StaticMeshDescription",
			// "GeometryCore",
			// "DynamicMesh",
			// "MeshConversion",
		});
	}
}
