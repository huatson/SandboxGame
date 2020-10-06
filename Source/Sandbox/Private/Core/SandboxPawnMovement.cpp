// Fill out your copyright notice in the Description page of Project Settings.


#include "SandboxPawnMovement.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "SandboxPawn.h"
#include "SandboxController.h"
#include "SandboxGameCamera.h"
#include "SandboxWeapon.h"



USandboxPawnMovement::USandboxPawnMovement(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
{
	TargetingSpeedModifier = 0.333f;
	WalkSpeedModifier = 0.333f;
	CrouchSpeedModifier = 0.50f;
	RunningSpeedModifier = 1.0f;
	RoadieRunSpeedModifier = 1.60f;
	BackwardMovementSpeedModifier = 0.75f;
	MaxStepHeight = 40.f;
	MaxWalkSpeed = 300.f;
	SetWalkableFloorZ(0.7f);

	RRCameraTurnTime = FVector2D(0.15f, 0.9f);
	bDisallowReverseTransitions = false;
	bCheckForCancelAtStart = true;
	MinSpeedPercent = 0.3f;
	MaxSlowTime = 0.5f;
	SteeringPct = 0.33f;
	bStopLoopingSoundsAtEndOfMove = true;
	LastRoadieRunTime = -9999.f;
	bAllowStoppingPower = true;
	MaxStoppingPower = 0.67;		
	StoppingPowerHeavyScale = 0.67;
	MaxStoppingPowerDistance = 2048.0;
	StoppingPowerAngle = -0.1;
	StoppingPowerRoadieRunStumbleThreshold = 1.0f;
	StoppingPower = 0.25f;
}


float USandboxPawnMovement::SandPawnDirection() const
{
	const FRotator DesiredRotation = GetPawnOwner() 
		? GetPawnOwner()->GetViewRotation() : GetOwner() 
		? GetOwner()->GetActorRotation() : FRotator::ZeroRotator;
	FQuat AQuat = FQuat(Velocity.Rotation());
	FQuat BQuat = FQuat(DesiredRotation.GetInverse());
	FRotator RawDirection = FRotator(AQuat*BQuat);
	RawDirection.Normalize();
	if (Velocity.Size() > 0)
	{
		if (RawDirection.Yaw >= 180.f)
		{
			return (RawDirection.Yaw - 360.f);
		}
		else
		{
			return RawDirection.Yaw;
		}
	}
	else
	{
		return 0;
	}
}

void USandboxPawnMovement::EndRoadieRunSteeringDelay()
{
	bDisableSteering = false;
}

void USandboxPawnMovement::RoadieRunMoveStarted()
{
	RunAbortTimer = 0.f;
	bDisableSteering = false;
	if (ASandboxController* PCOwner = Cast<ASandboxController>(PawnOwner->GetController()))
	{
		if (bCheckForCancelAtStart)
		{
			if (PCOwner->IsLocalPlayerController() && !PCOwner->IsHoldingRoadieRunButton())
			{
				RoadieRunMoveEnded();
				return;
			}
		}

		FVector X, Y, Z;
		UKismetMathLibrary::GetAxes(PCOwner->GetControlRotation(), X, Y, Z);
		ASandboxPawn* P = Cast<ASandboxPawn>(PawnOwner);
		if (!P)
		{
			RoadieRunMoveEnded();
			return;
		}
		P->GetWorld()->GetTimerManager().ClearTimer(P->TimerHandle_TryToRoadieRun);
		FVector Direction = (P->aForward * X) + (P->aStrafe * Y).GetSafeNormal();
		if (!bIgnoreCameraTurn)
		{
			if (!Direction.IsZero())
			{
				FRotator NewPlayerRot = Direction.Rotation();
				float YawDiff = FRotator::NormalizeAxis(PCOwner->GetControlRotation().Yaw - NewPlayerRot.Yaw);
				if (bDisallowReverseTransitions)
				{
					if ((YawDiff < -105.f) || (YawDiff > 105.f))
					{
						YawDiff = 0.f;
					}
					else
					{
						YawDiff = FMath::Clamp<float>(YawDiff, -90.f, 90.f);
					}

					NewPlayerRot.Yaw = PCOwner->GetControlRotation().Yaw - YawDiff;
				}
				PCOwner->SetControlRotation(NewPlayerRot);
				if (ASandboxGameCamera* SandCamera = Cast<ASandboxGameCamera>(PCOwner->PlayerCameraManager))
				{
					if (USandboxGameplayCamera* SandGameplayCamera = Cast<USandboxGameplayCamera>(SandCamera->GameplayCam))
					{
						const float Pct = FMath::Abs(YawDiff) / 180.0f;
						const float SqrtPct = FMath::Sqrt(Pct);
						const float CameraTurnTime = FMath::GetRangePct(RRCameraTurnTime, SqrtPct);
						SandGameplayCamera->BeginTurn(YawDiff, 0.f, CameraTurnTime, 0.f);
						bDisableSteering = true;
						PawnOwner->GetWorldTimerManager().SetTimer(TimerHandle_SteeringDelay, this, &USandboxPawnMovement::EndRoadieRunSteeringDelay, CameraTurnTime);
					}
				}
			}
		}
	}
}

void USandboxPawnMovement::RoadieRunMoveEnded()
{
	RoadieRunBoostTime = -9999.f;
	ASandboxController* PCOwner = Cast<ASandboxController>(PawnOwner->GetController());
	if (PCOwner)
	{
		PCOwner->ForcePitchCentering(true, true);
	}
	LastRoadieRunTime = PawnOwner->GetWorld()->GetTimeSeconds();
}

void USandboxPawnMovement::RoadieRunPreProcessInput(ASandboxPawn* SandPawn, ASandboxController* SandController)
{
	SandPawn->aForward = 1.0f;
	SandPawn->AddMovementInput(SandPawn->GetActorForwardVector() * SandPawn->aForward);
	const float YawValue = (bDisableSteering ? 0.f : (SandPawn->aStrafe * SteeringPct));
	SandController->RotationInput.Yaw += YawValue;
}

float USandboxPawnMovement::GetRoadierunSpeedModifier() const
{
	float BoostModifier = 0.f;
	if (PawnOwner)
	{
		if ((PawnOwner->GetWorld()->GetTimeSeconds() - RoadieRunBoostTime) < 1.5f)
		{
			BoostModifier = ((1.f - ((PawnOwner->GetWorld()->GetTimeSeconds() - RoadieRunBoostTime) / 1.5f)) * 0.5f);
		}
	}
	return (RoadieRunSpeedModifier + BoostModifier);
}

void USandboxPawnMovement::TickRoadieRun(float DeltaTime)
{
	if (Velocity.Size() < (GetMaxSpeed() * MinSpeedPercent))
	{
		RunAbortTimer += DeltaTime;
		if (RunAbortTimer > MaxSlowTime)
		{
			RoadieRunMoveEnded();
		}
	}
	else
	{
		RunAbortTimer = 0;
	}
}

void USandboxPawnMovement::AddStoppingPower(float InStoppingPower, FVector& Momentum, float DmgDistance)
{
	FStoppingPowerStruct NewStoppingPower;
	NewStoppingPower.StoppingPower = InStoppingPower * (1.0 - (DmgDistance / MaxStoppingPowerDistance));
	NewStoppingPower.Direction = Momentum.GetSafeNormal();
	NewStoppingPower.StoppingLifetime = 1.f;
	StoppingPowerList.Add(NewStoppingPower);
}

void USandboxPawnMovement::HandleStoppingPower(const FDamageEvent& DamageEvent, FVector Momentum, float DmgDistance)
{
	AddStoppingPower(StoppingPower, Momentum, DmgDistance);

}

float USandboxPawnMovement::GetResultingStoppingPower() const
{
	if (Velocity.SizeSquared2D() < 1.f)
	{
		return 0.f;
	}
	float AccumulatedStoppingPower = 0.f;

	for (int32 i = 0; i < StoppingPowerList.Num(); i++)
	{
		float DotScale = StoppingPowerList[i].Direction | Velocity.GetSafeNormal2D();
		if (DotScale < StoppingPowerAngle)
		{
			DotScale = 1.f - FMath::Square<float>(DotScale + 1.f);
			AccumulatedStoppingPower += StoppingPowerList[i].StoppingPower * DotScale * StoppingPowerList[i].StoppingLifetime;
		}
	}
	return FMath::Clamp<float>(AccumulatedStoppingPower, 0.f, FMath::Min<float>(MaxStoppingPower, 1.f));
}

float USandboxPawnMovement::GetGlobalSpeedModifier() const
{
	float GlobalSpeedModifier = 1.0f;
	ASandboxPawn* const P = Cast<ASandboxPawn>(GetPawnOwner());
	if (P)
	{
		if (P->bIsRoadieRunning)
		{
			GlobalSpeedModifier = GetRoadierunSpeedModifier();
		}
		else if (P->IsTargeting())
		{
			GlobalSpeedModifier = WalkSpeedModifier;
		}
		else if (P->bIsCrouched)
		{
			GlobalSpeedModifier = CrouchSpeedModifier;
		}
		else if (MovementMode == EMovementMode::MOVE_Walking && P->GetVelocity().Size2D() > MaxWalkSpeed * 0.1f)
		{
			const float DotP = (P->GetVelocity().GetSafeNormal2D() | P->GetActorRotation().Vector());
			if (DotP < -0.1f)
			{
				GlobalSpeedModifier = BackwardMovementSpeedModifier;
			}
		}
		if (!P->bIsInCombat)
		{
			GlobalSpeedModifier *= 0.85f;
		}

	}
	if (bAllowSpeedInterpolation)
	{
		float TmpResult = FMath::FInterpTo(LastMaxSpeedModifier, GlobalSpeedModifier, GetWorld()->GetDeltaSeconds(), 4.f);
		GlobalSpeedModifier = TmpResult;
	}

	const float ResultStoppingPower = GetResultingStoppingPower();
	if (ResultStoppingPower > 0.f)
	{
		GlobalSpeedModifier *= (1.f - StoppingPower);
	}

	//	Final modifier
	return GlobalSpeedModifier;
}

float USandboxPawnMovement::GetMaxSpeed() const
{
	return (Super::GetMaxSpeed()*GetGlobalSpeedModifier());
}

void USandboxPawnMovement::StartNewPhysics(float deltaTime, int32 Iterations)
{
	if (ASandboxPawn* const GP = Cast<ASandboxPawn>(GetPawnOwner()))
	{
		FVector OldLocation = GP->GetActorLocation();
		if ((MovementMode == EMovementMode::MOVE_Walking
			|| MovementMode == EMovementMode::MOVE_NavWalking)
			&& GP->bAllowAccelSmoothing
			&& GP->GetController() != nullptr
			&& !GP->IsPlayerControlled()
			&& !Acceleration.IsNearlyZero())
		{
			FVector AccelDir(FVector::ZeroVector);
			if (GP->OldAcceleration.IsNearlyZero())
			{
				AccelDir = GP->GetActorRotation().Vector();
			}
			else
			{
				AccelDir = GP->OldAcceleration.GetSafeNormal();
			}

			const float DesiredAccelSize = GetCurrentAcceleration().Size();
			FVector DesiredDir = GetCurrentAcceleration() / DesiredAccelSize;
			const float ConvergeTime = (2.0f * PI * GP->EffectiveTurningRadius) / FMath::Max<float>(Velocity.Size(), MaxWalkSpeed);
			const float RadPerSecondMax = ((2.0f * PI) / ConvergeTime) * deltaTime;
			float DeltaAngle = FMath::Acos(AccelDir | DesiredDir);
			DeltaAngle = FMath::Min<float>(DeltaAngle, RadPerSecondMax);
			float AccelDesDot = AccelDir | DesiredDir;
			FVector const FloorNormal = CurrentFloor.HitResult.Normal;
			float const AngleDeg = (DeltaAngle * 180.0f / PI) * 182.f;
			FVector const VectorAxis = (-AccelDesDot > 0.995f && -AccelDesDot < 1.01f) ? FloorNormal : AccelDir ^ DesiredDir;
			DesiredDir = AccelDir.RotateAngleAxis(AngleDeg, VectorAxis);
			checkSlow(!DesiredDir.ContainsNaN());
			Acceleration = DesiredDir * DesiredAccelSize;

			if (MovementMode == EMovementMode::MOVE_NavWalking)
			{
				FVector Y = FloorNormal ^ DesiredDir;
				FVector X = Y ^ FloorNormal;
			}
		}

		GP->OldAcceleration = GetCurrentAcceleration();
		Super::StartNewPhysics(deltaTime, Iterations);
		GP->PerformStepsSmoothing(OldLocation, deltaTime);
	}
	else
	{
		Super::StartNewPhysics(deltaTime, Iterations);
	}
}

void USandboxPawnMovement::PostPhysicsTickComponent(float DeltaTime, FCharacterMovementComponentPostPhysicsTickFunction& ThisTickFunction)
{
	Super::PostPhysicsTickComponent(DeltaTime, ThisTickFunction);
}
