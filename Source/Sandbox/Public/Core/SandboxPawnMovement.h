// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SandboxPawnMovement.generated.h"


class ASandboxPawn;
class ASandboxController;


USTRUCT()
struct FStoppingPowerStruct
{
	GENERATED_USTRUCT_BODY()

public:

	/** Constructors */
	FStoppingPowerStruct()
		: Direction(FVector::ZeroVector)
		, StoppingPower(0.0f)
		, StoppingLifetime(0.0f)
	{
	}

	FVector Direction;
	float StoppingPower;
	float StoppingLifetime;
};


UCLASS()
class SANDBOX_API USandboxPawnMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	USandboxPawnMovement(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, Category = "SandboxPawn")
	uint8 bAllowSpeedInterpolation : 1;
	float LastMaxSpeedModifier;

	/** modifier for targeting speed */
	UPROPERTY(EditDefaultsOnly, Category = "SandboxPawn")
	float TargetingSpeedModifier;

	/** modifier for walk speed */
	UPROPERTY(EditDefaultsOnly, Category = "SandboxPawn")
	float WalkSpeedModifier;

	/** modifier for crouch speed */
	UPROPERTY(EditDefaultsOnly, Category = "SandboxPawn")
	float CrouchSpeedModifier;

	/** modifier for Run movement speed */
	UPROPERTY(EditDefaultsOnly, Category = "SandboxPawn")
	float RunningSpeedModifier;

	/** modifier for RoadieRun movement speed */
	UPROPERTY(EditDefaultsOnly, Category = "SandboxPawn")
	float RoadieRunSpeedModifier;

	UPROPERTY(EditDefaultsOnly, Category = "SandboxPawn")
	float BackwardMovementSpeedModifier;

	FVector RampVector;
	float SlideDist;
	FVector StopMovementMarker;




	/**
	 * ROADIE MOVEMENT
	 */
	float RunAbortTimer;
	uint8 bCheckForCancelAtStart : 1;
	uint8 bStopLoopingSoundsAtEndOfMove : 1;
	uint8 bIgnoreCameraTurn : 1;
	float MinSpeedPercent;
	float MaxSlowTime;
	float SteeringPct;
	FTimerHandle TimerHandle_SteeringDelay;
	UPROPERTY(EditDefaultsOnly, Category = "Roadie Run")
	FVector2D RRCameraTurnTime;
	uint8 bDisableSteering : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Roadie Run")
	uint8 bDisallowReverseTransitions : 1;
	float RoadieRunBoostTime;
	float LastRoadieRunTime;
	UFUNCTION(BlueprintCallable, Category = "SandboxPawn")
	float SandPawnDirection() const;
	void EndRoadieRunSteeringDelay();
	virtual void RoadieRunMoveStarted();
	virtual void RoadieRunMoveEnded();
	virtual void RoadieRunPreProcessInput(ASandboxPawn* SandPawn, ASandboxController* SandController);
	float GetRoadierunSpeedModifier() const;
	void TickRoadieRun(float DeltaTime);



	TArray<FStoppingPowerStruct> StoppingPowerList;
	UPROPERTY(EditDefaultsOnly, Category = "StopPower")
	float MaxStoppingPower;
	UPROPERTY(EditDefaultsOnly, Category = "StopPower")
	float MaxStoppingPowerDistance;
	UPROPERTY(EditDefaultsOnly, Category = "StopPower")
	float StoppingPowerAngle;
	UPROPERTY(EditDefaultsOnly, Category = "StopPower")
	float StoppingPowerHeavyScale;
	UPROPERTY(EditDefaultsOnly, Category = "StopPower")
	float StoppingPowerRoadieRunStumbleThreshold;
	UPROPERTY(EditDefaultsOnly, Category = "StopPower")
	uint8 bAllowStoppingPower : 1;
	UPROPERTY(EditDefaultsOnly, Category = "StopPower")
	float StoppingPower;


	virtual void AddStoppingPower(float InStoppingPower, FVector& Momentum, float DmgDistance);
	virtual void HandleStoppingPower(const FDamageEvent& DamageEvent, FVector Momentum, float DmgDistance);
	virtual float GetResultingStoppingPower() const;
	virtual float GetGlobalSpeedModifier() const;
	virtual float GetMaxSpeed() const override;
	virtual void StartNewPhysics(float deltaTime, int32 Iterations) override;
	virtual void PostPhysicsTickComponent(float DeltaTime, FCharacterMovementComponentPostPhysicsTickFunction& ThisTickFunction) override;
};
