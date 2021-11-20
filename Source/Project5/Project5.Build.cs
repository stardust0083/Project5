// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Project5 : ModuleRules
{
	public Project5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
			{"Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "Slate"});
	}
}