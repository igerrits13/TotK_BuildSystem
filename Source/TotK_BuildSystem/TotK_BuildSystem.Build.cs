// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TotK_BuildSystem : ModuleRules
{
	public TotK_BuildSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
