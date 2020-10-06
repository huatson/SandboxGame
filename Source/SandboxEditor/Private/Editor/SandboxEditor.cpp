// Copyright Epic Games, Inc. All Rights Reserved.

#include "SandboxEditor.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FSandboxEditorModule, SandboxEditor);

DEFINE_LOG_CATEGORY(SandboxEditorLog)
#define LOCTEXT_NAMESPACE "FSandboxEditorModule"

void FSandboxEditorModule::StartupModule()
{
	UE_LOG(SandboxEditorLog, Warning, TEXT("FSandboxEditorModule: Startup Module"));
}

void FSandboxEditorModule::ShutdownModule()
{
	UE_LOG(SandboxEditorLog, Warning, TEXT("FSandboxEditorModule: Shutdown Module"));
}

#undef LOCTEXT_NAMESPACE
