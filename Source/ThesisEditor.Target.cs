using UnrealBuildTool;
using System.Collections.Generic;

public class ThesisEditorTarget : TargetRules
{
    public ThesisEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Thesis");
    }
}
