// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

using UnrealBuildTool;

public class Shadersource_Waterfall2Runtime : ModuleRules
{	public Shadersource_Waterfall2Runtime(ReadOnlyTargetRules Target) : base(Target)
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
                "Niagara",
                "GeometryFramework",
            }
            );

        //This is the Build.cs equivilent of "#if WITH_EDITOR"
        if (Target.bBuildEditor == true)
        {
            //Some of these are runtime modules but we only need them in editor-only content in this module
            PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "GeometryFramework",
				"GeometryScriptingCore",
                "GeometryScriptingEditor",
                "GeometryCore",
                "EditorScriptingUtilities", //For UEditorAssetLibrary
                "Projects",
                //"ComponentVisualizers",
				"StaticMeshDescription",
                "UnrealEd",      
				"LevelEditor"    
            }
            );
        }
    }
}