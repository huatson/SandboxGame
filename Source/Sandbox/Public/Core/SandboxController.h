// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SandboxController.generated.h"

class USoundCue;

/**
 * 
 */
UCLASS()
class SANDBOX_API ASandboxController : public APlayerController
{
	GENERATED_UCLASS_BODY()


	/** Method called prior to processing input */
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;

	/** POI */
	bool IsSpectating();
	FRotator ExtraRot;
	uint8 bLookingAtPointOfInterest : 1;
	UPROPERTY(EditDefaultsOnly, Category = "POI Settings")
	USoundCue*	PointOfInterestAdded;
	float LastPoITime;
	float PoIRepeatTime;
	virtual bool IsHoldingPOIButton(bool bRaw = false);



	virtual bool IsHoldingActionButton();
	virtual bool IsHoldingRoadieRunButton();
	virtual void UpdateRotation(float DeltaTime) override;
	bool IsInCoverState() const;



	/**
	 * PITCH CENTER
	 */
	uint8	bForcePitchCentering : 1;
	float	ForcePitchCenteringSpeed;
	UPROPERTY(EditDefaultsOnly, Category = "AutoPitch")
	float	DefaultForcePitchCenteringSpeed;
	float	ForcedPitchCenteringHorizonOffset;
	uint8	bForcePitchCenteringIsInterruptable : 1;
	UPROPERTY(EditDefaultsOnly, Category = "AutoPitch")
	uint8	bAutoCenterPitch : 1;
	UPROPERTY(EditDefaultsOnly, Category = "AutoPitch")
	float	PitchAutoCenterSpeed;
	UPROPERTY(EditDefaultsOnly, Category = "AutoPitch")
	float	PitchAutoCenterSpeedRoadieRun;
	UPROPERTY(EditDefaultsOnly, Category = "AutoPitch")
	float	PitchAutoCenterSpeedRoadieRunStab;
	UPROPERTY(EditDefaultsOnly, Category = "AutoPitch")
	float	PitchAutoCenterDelay;
	float PitchAutoCenterDelayCount;
	float PitchAutoCenterHorizonOffset;
	uint8 bPlayerInterruptedRoadieRunPitchAutoCentering : 1;
	UPROPERTY(EditDefaultsOnly, Category = "AutoPitch")
	FVector2D	PitchAutoCenterTargetPitchWindow;
	UPROPERTY(EditDefaultsOnly, Category = "AutoPitch")
	FVector2D	PitchAutoCenterTargetPitchWindow_RoadieRun;
	UPROPERTY(EditDefaultsOnly, Category = "AutoPitch")
	float RunWalkTransitionThreshold;
	float TargetingModeViewScalePct;

	void ForcePitchCentering(bool bCenter, bool bCancelOnUserInput = true, float GoalPitch = 0.f, float InterpSpeed = 10.f);
	void AutoPitchCentering(float DeltaTime);

};
