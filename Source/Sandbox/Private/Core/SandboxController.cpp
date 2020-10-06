// Fill out your copyright notice in the Description page of Project Settings.


#include "SandboxController.h"
#include "GameFramework/PlayerInput.h"
#include "SandboxGameCamera.h"
#include "SandboxPawn.h"
#include "SandboxPawnMovement.h"

ASandboxController::ASandboxController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerCameraManagerClass = ASandboxGameCamera::StaticClass();

	ForcePitchCenteringSpeed = 3.f;
	RunWalkTransitionThreshold = 0.75;
	TargetingModeViewScalePct = 0.455;  // 0.65 * 0.7
	bAutoCenterPitch = true;
	PitchAutoCenterDelay = 2.0;
	PitchAutoCenterSpeed = 0.5;
	PitchAutoCenterSpeedRoadieRun = 1.75;
	PitchAutoCenterSpeedRoadieRunStab = 4.0;
	PitchAutoCenterTargetPitchWindow = FVector2D::ZeroVector;
	PitchAutoCenterTargetPitchWindow_RoadieRun = FVector2D(-10.f, -10.f);
}

void ASandboxController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);
	ASandboxPawn* P = Cast<ASandboxPawn>(GetPawn());
	if(P)
	{
		if (P->bIsRoadieRunning)
		{
			P->SandboxMovementComponent->RoadieRunPreProcessInput(P, this);
		}

		if (bAutoCenterPitch)
		{
			AutoPitchCentering(DeltaTime);
		}
	}


}

bool ASandboxController::IsSpectating()
{
	return IsInState(EName::NAME_Spectating);
}

bool ASandboxController::IsHoldingPOIButton(bool bRaw)
{
	return PlayerInput && PlayerInput->IsPressed(EKeys::Q);
}


bool ASandboxController::IsHoldingActionButton()
{
	return PlayerInput && PlayerInput->IsPressed(EKeys::E);
}

bool ASandboxController::IsHoldingRoadieRunButton()
{
	return PlayerInput && PlayerInput->IsPressed(EKeys::LeftShift);
}


void ASandboxController::UpdateRotation(float DeltaTime)
{
	FRotator DeltaRot(RotationInput);
	ASandboxPawn* P = Cast<ASandboxPawn>(PlayerCameraManager->GetViewTargetPawn());
	ASandboxGameCamera* Camera = Cast<ASandboxGameCamera>(PlayerCameraManager);
	if (bLookingAtPointOfInterest && IsHoldingPOIButton(true))
	{
		if (P && Camera && ((P->aForward != 0.f || P->aStrafe != 0.f) && !IsInState(EName::NAME_Spectating)))
		{
			DeltaRot.Yaw = Camera->GameplayCam->LastYawAdjustment;
			DeltaRot.Pitch = Camera->GameplayCam->LastPitchAdjustment;
		}
	}
	else
	{
		DeltaRot.Yaw = P->aTurn;
		DeltaRot.Pitch = P->aLookUp;
		
		if (GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation() < 1.f)
		{
			DeltaRot *= GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
		}

		DeltaRot += ExtraRot;
		ExtraRot = FRotator::ZeroRotator;
	}
	Super::UpdateRotation(DeltaTime);
}

bool ASandboxController::IsInCoverState() const
{
	return false;
}

void ASandboxController::ForcePitchCentering(bool bCenter, bool bCancelOnUserInput /*= true*/, float GoalPitch /*= 0.f*/, float InterpSpeed /*= 10.f*/)
{
	bForcePitchCentering = bCenter;
	bForcePitchCenteringIsInterruptable = bCancelOnUserInput;
	ForcedPitchCenteringHorizonOffset = GoalPitch;
	ForcePitchCenteringSpeed = (InterpSpeed == 0.f) ? DefaultForcePitchCenteringSpeed : InterpSpeed;
}

void ASandboxController::AutoPitchCentering(float DeltaTime)
{
	float CurrentPitch = FRotator::NormalizeAxis(GetControlRotation().Pitch);
	ASandboxPawn* SP = Cast<ASandboxPawn>(GetPawn());
	if (!SP)
		return;

	const bool bRoadieRunning = SP->bIsRoadieRunning;
	if (bRoadieRunning)
	{
		if (RotationInput.Pitch != 0)
		{
			bPlayerInterruptedRoadieRunPitchAutoCentering = true;
		}
	}
	else
	{
		bPlayerInterruptedRoadieRunPitchAutoCentering = false;
	}

	if (bForcePitchCenteringIsInterruptable && (RotationInput.Pitch != 0.f))
	{
		bForcePitchCentering = false;
	}

	if (!bForcePitchCentering)
	{
		if (IsInCoverState())
		{
			if ((RotationInput.Pitch != 0.f)
				|| (GetPawn()->GetVelocity().Size2D() < 1.f)
				|| (SP->IsTargeting()))
			{
				PitchAutoCenterDelayCount = 0.f;
				return;
			}
		}
		else
			if ((RotationInput.Pitch != 0.f)
				|| ((SP->aForward == 0.f) && (SP->aStrafe == 0.f))
				|| (GetPawn()->GetVelocity().Size2D() < 1.f))
			{
				PitchAutoCenterDelayCount = 0.f;
				return;
			}

		if ((PitchAutoCenterDelayCount < PitchAutoCenterDelay) && (!bRoadieRunning || bPlayerInterruptedRoadieRunPitchAutoCentering))
		{
			PitchAutoCenterDelayCount += DeltaTime;
			return;
		}
	}

	FVector2D PitchWindow = bRoadieRunning ? PitchAutoCenterTargetPitchWindow_RoadieRun : PitchAutoCenterTargetPitchWindow;
	float PitchWindowOffset = 0.f;
	if (bRoadieRunning && !bPlayerInterruptedRoadieRunPitchAutoCentering)
	{
		FRotationMatrix R(GetControlRotation());
		FVector PawnX = R.GetUnitAxis(EAxis::X);
		FVector PawnY = R.GetUnitAxis(EAxis::Y);
		FVector PawnZ = R.GetUnitAxis(EAxis::Z);

		FVector SurfaceUp = SP->GetCharacterMovement()->CurrentFloor.HitResult.Normal;
		FVector SurfaceRight = FVector::CrossProduct(SurfaceUp, PawnX);
		FVector SurfaceFwd = FVector::CrossProduct(SurfaceUp, SurfaceRight);
		FMatrix M;
		M.SetIdentity();
		M.SetAxis(0, SurfaceFwd);
		M.SetAxis(1, SurfaceRight);
		M.SetAxis(2, SurfaceUp);
		FRotator SurfaceRot = M.Rotator();
		PitchWindowOffset = -SurfaceRot.Pitch;
	}
	PitchWindowOffset += PitchAutoCenterHorizonOffset;
	if (bForcePitchCentering)
	{
		PitchWindowOffset += ForcedPitchCenteringHorizonOffset;
	}
	PitchWindow.X = FRotator::NormalizeAxis(PitchWindow.X + PitchWindowOffset);
	PitchWindow.Y = FRotator::NormalizeAxis(PitchWindow.Y + PitchWindowOffset);
	float TargetPitch = 0.f;
	if (PitchWindow.Y < CurrentPitch)
	{
		TargetPitch = PitchWindow.Y;
	}
	else if (PitchWindow.X > CurrentPitch)
	{
		TargetPitch = PitchWindow.X;
	}
	else
	{
		return;
	}
	float DeltaPitch = 0.f;
	if (bForcePitchCentering)
	{
		DeltaPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, ForcePitchCenteringSpeed) - CurrentPitch;
	}
	else if (bRoadieRunning && !bPlayerInterruptedRoadieRunPitchAutoCentering)
	{
		DeltaPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, PitchAutoCenterSpeedRoadieRun) - CurrentPitch;
	}
	else
	{
		if (SP->IsTargeting())
		{
			DeltaPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, PitchAutoCenterSpeed * 0.5f) - CurrentPitch;
		}
		else
		{
			DeltaPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, PitchAutoCenterSpeed) - CurrentPitch;
		}
	}
	RotationInput.Pitch += DeltaPitch;
}

