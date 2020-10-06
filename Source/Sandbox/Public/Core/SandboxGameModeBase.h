// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SandboxGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class SANDBOX_API ASandboxGameModeBase : public AGameModeBase
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Foot Placement Updates.
	 */

	UPROPERTY(Transient)
	float FootPlacementLineChecksTimeStamp;
	UPROPERTY(EditDefaultsOnly, Category = "FootPlacement ")
	uint32 MaxFootPlacementLineChecksPerFrame;
	UPROPERTY(Transient)
	uint32 FootPlacementUpdateTag;
	UPROPERTY(Transient)
	uint32 NumFootPlacementUpdatesThisFrame;


	/**
	 * Floor Conform Updates
	 */
	UPROPERTY(Transient)
	float FloorConformTimeStamp;
	UPROPERTY(Transient)
	uint32 NumFloorConformUpdatesThisFrame;
	UPROPERTY(EditDefaultsOnly, Category = "FloorConform")
	uint32 MaxFloorConformUpdatesPerFrame;
	uint32 FloorConformUpdateTag;
};
