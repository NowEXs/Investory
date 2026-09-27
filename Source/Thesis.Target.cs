using UnrealBuildTool;
using System.Collections.Generic;

public class ThesisTarget : TargetRules
{
    public ThesisTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Thesis");
    }
}
