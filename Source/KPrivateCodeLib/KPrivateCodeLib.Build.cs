using UnrealBuildTool;

public class KPrivateCodeLib : ModuleRules
{
	public KPrivateCodeLib(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Json",
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"PhysicsCore",
				"InputCore",
				"OnlineSubsystem", "OnlineSubsystemNull", "OnlineSubsystemUtils",
				"SignificanceManager",
				"GeometryCollectionEngine",
				"ChaosVehiclesCore", "ChaosVehicles", "ChaosSolverEngine",
				"AnimGraphRuntime",
				"AkAudio",
				"AssetRegistry",
				"NavigationSystem",
				"ReplicationGraph",
				"AIModule",
				"GameplayTasks",
				"SlateCore", "Slate", "UMG",
				"InstancedSplines",
				"RenderCore",
				"CinematicCamera",
				"Foliage",
				"Niagara",
				"EnhancedInput",
				"GameplayCameras",
				"TemplateSequence",
				"NetCore", "HTTP",
				"GameplayTags"
			});

		// FactoryGame plugins
		PublicDependencyModuleNames.AddRange(new[]
		{
			"AbstractInstance",
			"InstancedSplinesComponent",
			"SignificanceISPC"
		});

		// Header stubs
		PublicDependencyModuleNames.AddRange(new[]
		{
			"DummyHeaders"
		});

		if (Target.Type == TargetRules.TargetType.Editor)
			PublicDependencyModuleNames.AddRange(new[] { "OnlineBlueprintSupport", "AnimGraph" });
		PublicDependencyModuleNames.AddRange(new[] { "FactoryGame", "SML", "KBFL" });
	}
}