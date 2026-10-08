// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GSND6250_LevelDesign : ModuleRules
{
	public GSND6250_LevelDesign(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"GSND6250_LevelDesign",
			"GSND6250_LevelDesign/Variant_Platforming",
			"GSND6250_LevelDesign/Variant_Platforming/Animation",
			"GSND6250_LevelDesign/Variant_Combat",
			"GSND6250_LevelDesign/Variant_Combat/AI",
			"GSND6250_LevelDesign/Variant_Combat/Animation",
			"GSND6250_LevelDesign/Variant_Combat/Gameplay",
			"GSND6250_LevelDesign/Variant_Combat/Interfaces",
			"GSND6250_LevelDesign/Variant_Combat/UI",
			"GSND6250_LevelDesign/Variant_SideScrolling",
			"GSND6250_LevelDesign/Variant_SideScrolling/AI",
			"GSND6250_LevelDesign/Variant_SideScrolling/Gameplay",
			"GSND6250_LevelDesign/Variant_SideScrolling/Interfaces",
			"GSND6250_LevelDesign/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
