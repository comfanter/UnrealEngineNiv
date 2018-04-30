﻿// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class TrueSkyEditorPlugin : ModuleRules
	{
		public TrueSkyEditorPlugin(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				}
				);
			
				PublicIncludePaths.AddRange(new string[] {
					"Editor/LevelEditor/Public",
					"Editor/PlacementMode/Private",
					"Editor/MainFrame/Public/Interfaces",
                    "Developer/AssetTools/Private",
                    "TrueSkyPlugin/Private",
                    "TrueSkyPlugin/Public",
                    "TrueSkyPlugin/Classes",
                    "TrueSkyEditorPlugin/Private",
                    "TrueSkyEditorPlugin/Public",
                    "TrueSkyEditorPlugin/Classes",
				});
			

			// ... Add private include paths required here ...
			PrivateIncludePaths.AddRange(
					new string[] {
						"TrueSkyEditorPlugin/Private"
						,"TrueSkyPlugin/Private"
						,"TrueSkyPlugin/Public"
						,"TrueSkyPlugin/Classes"
					}
				);

			// Add public dependencies that we statically link with here ...
			PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"Core",
						"CoreUObject",
						"Slate",
						"Engine",
					}
				);

				PublicDependencyModuleNames.AddRange(
						new string[]
						{
							"UnrealEd",
							"EditorStyle",
							"CollectionManager",
							"EditorStyle",
							"AssetTools",
							"PlacementMode",
							"ContentBrowser",
							"TrueSkyPlugin"
						}
					);
			

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"RenderCore",
                    "RHI",
                    "D3D11RHI",
					"Slate",
					"SlateCore",
					"InputCore",
                    "Renderer",
				"DesktopPlatform"
					// ... add private dependencies that you statically link with here ...
				}
				);

            AddEngineThirdPartyPrivateStaticDependencies(Target,
				
				"DX11"
				
				);

			PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		}
	}
}
