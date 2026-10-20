// Copyright (C) 2025 nulled.softworks. All rights reserved.

using UnrealBuildTool;

public class NsSessionSteam : ModuleRules
{
    public NsSessionSteam(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Engine modules and the engine plugin Steam Sockets only. This module does not need the NsSession core
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "SteamSockets",
        });
    }
}
