using UnrealBuildTool;
using System.IO;
using System;

public class KPrivateCodeLib : ModuleRules
{
	public KPrivateCodeLib(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject",
			"Engine",
			"InputCore",
			"OnlineSubsystem", "OnlineSubsystemUtils", "OnlineSubsystemNULL",
			"SignificanceManager",
			"PhysX", "APEX", "PhysXVehicles", "ApexDestruction",
			"AkAudio",
			"ReplicationGraph",
			"HTTP", "RHI", "UMG",
			"AIModule",
			"NavigationSystem",
			"AssetRegistry",
			"GameplayTasks",
			"AnimGraphRuntime",
			"Slate", "SlateCore",
			"Json", "KBFL", "RenderCore", "AbstractInstance"
		});


		if (Target.Type != TargetType.Server)
		{
			PrivateIncludePathModuleNames.AddRange(
				new string[]
				{
					"SlateRHIRenderer"
				}
			);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					"ImageWrapper",
					"SlateRHIRenderer"
				}
			);
		}

		if (Target.Type == TargetRules.TargetType.Editor)
			PublicDependencyModuleNames.AddRange(new string[] { "OnlineBlueprintSupport", "AnimGraph" });
		PublicDependencyModuleNames.AddRange(new string[] { "FactoryGame", "SML", "AbstractInstance" });
	}
}