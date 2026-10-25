using UnrealBuildTool;
using System.Collections.Generic;

public class VelocityTowerEditorTarget : TargetRules
{
	public VelocityTowerEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("VelocityTower");
	}
}
