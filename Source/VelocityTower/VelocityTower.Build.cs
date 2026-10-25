using UnrealBuildTool;

public class VelocityTower : ModuleRules
{
	public VelocityTower(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"CinematicCamera",
			"Niagara",
			"UMG",
			"AudioMixer",
			"SignalProcessing"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"ImageWrapper"
		});
	}
}
