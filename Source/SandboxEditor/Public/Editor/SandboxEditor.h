#pragma once

#include "CoreMinimal.h"
#include "UnrealEd.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(SandboxEditorLog, All, All)

class FSandboxEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};