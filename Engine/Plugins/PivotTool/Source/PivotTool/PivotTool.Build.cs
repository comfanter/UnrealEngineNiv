// Copyright 2018 BruceLee, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PivotTool : ModuleRules
{
	public PivotTool(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				"PivotTool/Public"
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"PivotTool/Private",
                "PivotTool/Private/EdMode",
                "PivotTool/Private/Widgets",
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
                "DetailCustomizations",
                "Engine",
                "EditorStyle",
                "PropertyEditor",
                "Projects",
                "Settings",
                "Slate",
                "SlateCore",
                "ImageWrapper",
                "InputCore",
                "LevelEditor",
                "RawMesh",
                "RenderCore",
                "UnrealEd",
                "WorkspaceMenuStructure",
				// ... add private dependencies that you statically link with here ...	
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
