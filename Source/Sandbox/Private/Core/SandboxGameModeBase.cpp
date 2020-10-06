// Copyright Epic Games, Inc. All Rights Reserved.


#include "SandboxGameModeBase.h"

ASandboxGameModeBase::ASandboxGameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MaxFootPlacementLineChecksPerFrame = 2;
	MaxFloorConformUpdatesPerFrame = 2;
}

