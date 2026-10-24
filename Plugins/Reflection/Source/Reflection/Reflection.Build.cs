/* Copyright Reflection Contributors 2024-2026 */

using System;
using System.IO;
using UnrealBuildTool;

/* NOTE: Please make sure to put UE5 only modules in the #if statement below, we want UE4 and UE5 compatibility */
public class Reflection : ModuleRules {
	public Reflection(ReadOnlyTargetRules Target) : base(Target)  {
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		var bIsLinux = Target.Platform == UnrealTargetPlatform.Linux;

		/* Nothing reads this any more: the way in is the Direct Asset Data setting */
		var bCloudServer = true;

		PublicDefinitions.Add("REFLECTION_CLOUD_SERVER=" + (bCloudServer ? "1" : "0"));

		/* This project does not use rig reconstruction, DNA facial animation, or curve expressions.
		 * Compile them out: their newer engine APIs are not compatible with UE 5.3. */
		var bControlRig = false;
		var bRigVM = false;
		var bRigLogic = false;
		var bCurveExpression = false;

		PublicDefinitions.Add("REFLECTION_CONTROL_RIG=0");
		PublicDefinitions.Add("REFLECTION_RIGVM=0");
		PublicDefinitions.Add("REFLECTION_RIG_LOGIC=0");
		PublicDefinitions.Add("REFLECTION_DNA_USER_DATA=0");
		PublicDefinitions.Add("REFLECTION_RIG_LOGIC_UE_SPACE_READER=0");
		PublicDefinitions.Add("REFLECTION_CURVE_EXPRESSION=0");

#if UE_5_0_OR_LATER
	    /* Unreal Engine 5 and later */
	    CppStandard = CppStandardVersion.Cpp20;
#else
		/* Unreal Engine 4 */
		CppStandard = CppStandardVersion.Cpp17;

#if !UE_4_26_OR_LATER
		/* The engine's shared PCH is built at the engine default standard on these versions, and
		 * MSVC refuses to consume a PCH compiled under a different /std. The sources are already
		 * include-what-you-use, so dropping the PCH entirely costs build time and nothing else. */
		PCHUsage = PCHUsageMode.NoPCHs;
#endif
#endif

		PublicDependencyModuleNames.AddRange(new[] {
			"Core",
			"Json",
			"JsonUtilities",
			"UMG",
			"RenderCore",
			"HTTP",
			"Niagara",
			"UnrealEd",
			"MainFrame",
			"AIModule",
				"GameplayTags",
			"ApplicationCore",
			"AnimGraph",
			"UMGEditor",
			"MovieScene",

#if UE_4_26_OR_LATER
			/* UDeveloperSettings lived inside Engine, at the same header path, until 4.26 gave
			 * it a module of its own */
			"DeveloperSettings",
#endif

#if UE_4_24_OR_LATER
			/* The cloth runtime was a single ClothingSystemRuntime module until 4.24 split it */
			"ClothingSystemRuntimeCommon",
#else
			"ClothingSystemRuntime",
#endif

#if UE_5_0_OR_LATER
			"ContentBrowserData",

			/* FBSPOps, which built brushes from inside UnrealEd until UE5 moved it out */
			"BSPUtils"
#endif
		});

		PrivateDependencyModuleNames.AddRange(new[] {
			"Projects",
			"InputCore",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"MaterialEditor",

			/* SObjectPropertyEntryBox and the rest of the editor's asset pickers */
			"PropertyEditor",

			/* The node classes a Niagara graph is made of, which live beside the editor rather
			 * than in Niagara itself and are not loaded unless something asks for them */
			"NiagaraEditor",
			"Landscape",
			"AssetTools",
			"EditorStyle",

			/* FContentBrowserModule, for the selection behind the right click menu */
			"ContentBrowser",
			"Settings",
			"RHI",
			"Detex",
			"NVTT",
			"RenderCore",
			"AnimGraphRuntime",
			"AnimGraph",

			/* FNodeFactory/SGraphNode, used to measure anim graph nodes when auto-laying them out */
			"GraphEditor",

			/* UEdGraphSchema_K2, whose PC_* pin categories name the type of a blueprint variable */
			"BlueprintGraph",

			/* AInstancedFoliageActor and what it plants */
			"Foliage",

			/* FMeshDescription, which the static mesh importer describes its geometry into */
			"MeshDescription",

#if UE_4_24_OR_LATER
			/* FStaticMeshAttributes, which 4.24 split out of MeshDescription */
			"StaticMeshDescription",
#endif

#if UE_4_23_OR_LATER
			/* PhysicsCore was carved out of Engine in 4.23 */
			"PhysicsCore",
#endif

#if UE_4_24_OR_LATER
			/* FKismetBytecodeDisassembler lived in UnrealEd until 4.24 gave it a module of its own */
			"ScriptDisassembler",
#endif

#if UE_4_24_OR_LATER
			/* ToolMenus is what replaced the level editor's FExtender based toolbar */
			"ToolMenus",
#endif

#if UE_4_25_OR_LATER
			"AudioModulation",
#endif

#if UE_4_26_OR_LATER
			"PluginUtils",
#endif

#if UE_5_0_OR_LATER
			/* Only Unreal Engine 5 */

			"AnimationDataController",
			"ToolWidgets"
#endif
		});
		
		/* Where the rig importer is not built, these are still what a graph is drawn with */
		if (bRigVM && !bControlRig) {
			PrivateDependencyModuleNames.AddRange(new[] {
				"ControlRig",
				"ControlRigDeveloper",
				"RigVMDeveloper"
			});
		}

		if (bControlRig) {
			PrivateDependencyModuleNames.AddRange(new[] {
				/* URigHierarchy and the controller every element is added through */
				"ControlRig",

				/* UControlRigBlueprint, and URigVMBlueprint under it */
				"ControlRigDeveloper",
				"RigVMDeveloper",

				/* The VM itself: its function registry is what names the node behind an instruction */
				"RigVM",

				/* UControlRigBlueprintFactory, which is what gives a new rig its graph */
				"ControlRigEditor"
			});
		}

		if (bRigLogic) {
			PrivateDependencyModuleNames.AddRange(new[] {
				/* UDNAAsset, and the reader that turns a DNA stream into one */
				"RigLogicModule",

				/* The DNA reader and writer themselves, for putting one into the axes the anim
				 * node reads */
				"RigLogicLib",

				/* UAnimGraphNode_RigLogic, which is how the node is put in a graph to test it */
				"RigLogicDeveloper"
			});
		}

		if (bCurveExpression) {
			PrivateDependencyModuleNames.AddRange(new[] {
				/* UCurveExpressionsDataAsset, and the list its expressions are written into */
				"CurveExpression"
			});
		}

		if (!bIsLinux) {
			PrivateDependencyModuleNames.AddRange(new[] {
				"Detex",
				"NVTT"
			});
		}
	}
}