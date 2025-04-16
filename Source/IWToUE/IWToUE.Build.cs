// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class IWToUE : ModuleRules
{
	public IWToUE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		var engineRoot = Path.GetFullPath(Target.RelativeEnginePath);

		PublicIncludePaths.AddRange(
			new string[]
			{
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"PhysicsCore",
				"PhysicsUtilities", 
				"OodleDataCompression",
				"SQLiteCore",
				"SQLiteSupport",
				"Projects"
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"MeshDescription",
				"StaticMeshDescription",
				"SkeletalMeshDescription",
				"SkeletalMeshUtilitiesCommon",
				"InputCore",
				"ToolMenus",
				"EditorStyle",
				"Json",
				"JsonUtilities",
				"ToolWidgets",
				"ApplicationCore",
				"MessageLog",
				"HTTP"
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}