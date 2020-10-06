// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SandboxEditor : ModuleRules
{
	public SandboxEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine",
			"AnimGraph",
			"BlueprintGraph",
			"Sandbox"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
		
		 PublicIncludePaths.AddRange(new string[]
		{
			"SandboxEditor/Public/Editor",
			"SandboxEditor/Public/Animation"
		});

		PrivateIncludePaths.AddRange(new string[]
		{
			"SandboxEditor/Private/Editor",
			"SandboxEditor/Private/Animation"
		});
	}
}
