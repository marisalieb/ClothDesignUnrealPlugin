using UnrealBuildTool;

public class ClothDesign : ModuleRules
{
	public ClothDesign(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
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
			new []
			{
				"Core",
				"GeometryFramework", 
				"MeshDescription", 
				"StaticMeshDescription",
				"ProceduralMeshComponent",
				"GeometryCore",
				"ChaosCloth",
				"MeshModelingTools",
				"ModelingOperators",
				"EditorScriptingUtilities"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new []
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"EditorFramework",
				"EditorStyle",
				"UnrealEd",
				"LevelEditor",
				"InteractiveToolsFramework",
				"EditorInteractiveToolsFramework",
				"ToolMenus",
				"PropertyEditor",
				"Projects",
				"GeometryAlgorithms",
				"DynamicMesh",
				"ContentBrowser",
				"ClothingSystemRuntimeInterface",
				"ClothingSystemRuntimeCommon",
				"MeshConversion",
				"GeometryScriptingEditor",
				"GeometryScriptingCore", 
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that the module loads dynamically here ...
			}
			);
	}
}
