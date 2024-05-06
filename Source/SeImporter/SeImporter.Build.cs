using System.IO;
using UnrealBuildTool;

public class SeImporter : ModuleRules
{
	public SeImporter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		var engineRoot = Path.GetFullPath(Target.RelativeEnginePath);

		PublicIncludePaths.AddRange(
			new string[]
			{
				/*Path.Combine(engineRoot, "Plugins", "Experimental", "Animation", "SkeletalMeshModelingTools", "Source",
					"SkeletalMeshModelingTools", "Private")*/
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add other private include paths required here ...
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"PhysicsCore",
				"PhysicsUtilities"
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
				"SkeletalMeshUtilitiesCommon",
				"InputCore",
				"ToolMenus",
				"EditorStyle",
				"Json",
				"JsonUtilities"
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
