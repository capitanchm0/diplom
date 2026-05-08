// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

using UnrealBuildTool;

public class Shadersource_Waterfall2EdMode : ModuleRules
{
	public Shadersource_Waterfall2EdMode(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Shadersource_Waterfall2Runtime",
                "ToolMenus",
                "InputCore",
				"EditorFramework",
				"EditorStyle",
                "Projects",
                "UnrealEd",
				"LevelEditor",
				"InteractiveToolsFramework",
				"EditorInteractiveToolsFramework",
                "MainFrame", //IMainFrameModule
				"GeometryScriptingCore",
                "GeometryScriptingEditor",
                "GeometryFramework",
				"GeometryCore",
				"ContentBrowser",
                "EditorScriptingUtilities",
                "Niagara",
                "ComponentVisualizers",
                "EditorWidgets",
                "DeveloperSettings",
            }
            );
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
