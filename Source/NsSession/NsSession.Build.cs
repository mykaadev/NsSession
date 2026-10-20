// Copyright (C) 2025 nulled.softworks. All rights reserved.

using UnrealBuildTool;

public class NsSession : ModuleRules
{
    public NsSession(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Engine modules and the engine online plugins only. NsSession depends on no studio plugin
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "CoreOnline",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
        });

        // The worldwide room search calls Steamworks directly. Other platforms skip it
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
            PrivateDefinitions.Add("NSSESSION_WITH_STEAM=1");
        }
        else
        {
            PrivateDefinitions.Add("NSSESSION_WITH_STEAM=0");
        }
    }
}
