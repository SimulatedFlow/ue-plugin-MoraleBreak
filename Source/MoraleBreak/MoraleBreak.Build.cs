// Copyright 2026 Silvan Teufel. All Rights Reserved.

using UnrealBuildTool;

public class MoraleBreak : ModuleRules
{
	public MoraleBreak(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// One runtime module. Morale is a thing the player watches happen; none of it may be editor-only.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",

			// AActor, UActorComponent, UTickableWorldSubsystem, DrawDebugHelpers for MoraleBreak.Debug.
			"Engine",

			// UMoraleBreakSettings is a UDeveloperSettings, so the thresholds, the hysteresis band and the
			// event weight table sit under Project Settings > Plugins > MoraleBreak with no editor module.
			"DeveloperSettings",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});

		// Deliberately NOT here:
		//   AIModule          - morale decides how an agent FEELS, never where it walks. Binding this to a
		//                       behaviour tree would make the plugin an opinion about how AI is structured,
		//                       and projects on StateTree, on GAS or on hand-written controllers would all
		//                       have to fight it. The state change is a delegate; what it makes an agent do
		//                       is the project's decision.
		//   GameplayAbilities - the same. Morale here is a float on a component, not an attribute set.
	}
}
