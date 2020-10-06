
#include "SandboxCameraManager.h"
#include "Engine/Canvas.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/MovementComponent.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(SandboxCameraLog)

#define DEBUGCAMERA_PENETRATION 0

/**
* UGameCameraBase
**/
UGameCameraBase::UGameCameraBase()
{

}

void UGameCameraBase::OnBecomeActive(UGameCameraBase* OldCamera)
{

}

void UGameCameraBase::OnBecomeInActive(UGameCameraBase* NewCamera)
{

}

void UGameCameraBase::UpdateCamera(APawn* P, ASandboxCameraManager* CameraActor, float DeltaTime, FTViewTarget& OutVT)
{

}

void UGameCameraBase::ResetInterpolation()
{
	bResetCameraInterpolation = true;
}

void UGameCameraBase::Init()
{

}

void UGameCameraBase::ProcessViewRotation(float DeltaTime, AActor* ViewTarget, FRotator& out_ViewRotation, FRotator& out_DeltaRot)
{

}

void UGameCameraBase::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.SetDrawColor(FColor::Magenta);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("GameCameraBase %s"), *GetNameSafe(this)));
}



/**
* UGameFixedCamera
**/
UGameFixedCamera::UGameFixedCamera()
{
	DefaultFOV = 80.f;
}

void UGameFixedCamera::OnBecomeActive(UGameCameraBase* OldCamera)
{
	bResetCameraInterpolation = true;
	Super::OnBecomeActive(OldCamera);
}

void UGameFixedCamera::UpdateCamera(APawn* P, ASandboxCameraManager* CameraActor, float DeltaTime, FTViewTarget& OutVT)
{
	ACameraActor* CamActor = Cast<ACameraActor>(OutVT.Target);
	if (CamActor)
	{
		OutVT.POV.FOV = CamActor->GetCameraComponent()->FieldOfView;
	}
	else
	{
		OutVT.POV.FOV = DefaultFOV;
	}

	if (OutVT.Target && CamActor)
	{
		OutVT.POV.Location = CamActor->GetCameraComponent()->GetComponentLocation();
		OutVT.POV.Rotation = CamActor->GetCameraComponent()->GetComponentRotation();
	}
	PlayerCamera->ApplyCameraModifiers(DeltaTime, OutVT.POV);
	bResetCameraInterpolation = false;
}



/**
* UGameThirdPersonCameraMode
**/
UGameThirdPersonCameraMode::UGameThirdPersonCameraMode()
{
	BlendTime = 0.67f;

	bLockedToViewTarget = true;
	bDoPredictiveAvoidance = true;
	bValidateWorstLoc = true;
	bSkipCameraCollision = false;
	bInterpLocation = true;
	OriginLocInterpSpeed = 8.f;
	StrafeLeftAdjustment = FVector::ZeroVector;
	StrafeRightAdjustment = FVector::ZeroVector;
	StrafeOffsetInterpSpeedIn = 12.f;
	StrafeOffsetInterpSpeedOut = 20.f;
	RunFwdAdjustment = FVector::ZeroVector;
	RunBackAdjustment = FVector::ZeroVector;
	RunOffsetInterpSpeedIn = 6.f;
	RunOffsetInterpSpeedOut = 12.f;
	WorstLocOffset = FVector(-8.f, 1.f, 90.f);
	bInterpViewOffsetOnlyForCamTransition = true;
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_Full].OffsetHigh = FVector::ZeroVector;
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_Full].OffsetLow = FVector::ZeroVector;
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_Full].OffsetMid = FVector::ZeroVector;
	OffsetAdjustmentInterpSpeed = 12.f;
}

void UGameThirdPersonCameraMode::SetViewOffset(const FViewOffsetData& NewViewOffset)
{
	ViewOffset = NewViewOffset;
}

FVector UGameThirdPersonCameraMode::GetCameraWorstCaseLoc(APawn* TargetPawn, FTViewTarget& CurrentViewTarget)
{
	return TargetPawn->GetActorLocation() + TargetPawn->GetActorRotation().RotateVector(WorstLocOffset);
}

FVector UGameThirdPersonCameraMode::AdjustViewOffset(APawn* P, FVector& Offset)
{
	return Offset;
}

FVector UGameThirdPersonCameraMode::GetViewOffset(APawn* ViewedPawn, float DeltaTime, const FVector& ViewOrigin, const FRotator& ViewRotation)
{
	FVector out_Offset(FVector::ZeroVector);
	CurrentViewportType = ECameraViewportTypes::CVT_16to9_Full;
	{
		UGameViewportClient* VPClient = nullptr;
		ULocalPlayer* const LP = (ThirdPersonCam->PlayerCamera->PCOwner != nullptr) ? Cast<ULocalPlayer>(ThirdPersonCam->PlayerCamera->PCOwner->Player) : nullptr;
		VPClient = (LP != nullptr) ? LP->ViewportClient : nullptr;
		if (VPClient != nullptr)
		{
			bool bWideScreen = false;
			{
				FVector2D ViewportSize;
				VPClient->GetViewportSize(ViewportSize);
				float Aspect = ViewportSize.X / ViewportSize.Y;
				if ((Aspect > (16.f / 9.f - 0.01f)) && (Aspect < (16.f / 9.f + 0.01f)))
				{
					bWideScreen = true;
				}
			}
			ESplitScreenType::Type CurrentSplitType = VPClient->GetCurrentSplitscreenConfiguration();
			if (bWideScreen)
			{
				if (CurrentSplitType == ESplitScreenType::TwoPlayer_Vertical)
				{
					CurrentViewportType = CVT_16to9_VertSplit;
				}
				else if (CurrentSplitType == ESplitScreenType::TwoPlayer_Horizontal)
				{
					CurrentViewportType = CVT_16to9_HorizSplit;
				}
				else
				{
					CurrentViewportType = CVT_16to9_Full;
				}
			}
			else
			{
				if (CurrentSplitType == ESplitScreenType::TwoPlayer_Vertical)
				{
					CurrentViewportType = CVT_4to3_VertSplit;
				}
				else if (CurrentSplitType == ESplitScreenType::TwoPlayer_Horizontal)
				{
					CurrentViewportType = CVT_4to3_HorizSplit;
				}
				else
				{
					CurrentViewportType = CVT_4to3_Full;
				}
			}
		}
	}
	FVector MidOffset(FVector::ZeroVector);
	FVector LowOffset(FVector::ZeroVector);
	FVector HighOffset(FVector::ZeroVector);
	{
		GetBaseViewOffsets(ViewedPawn, CurrentViewportType, DeltaTime, LowOffset, MidOffset, HighOffset);
		LowOffset += ViewOffset_ViewportAdjustments[CurrentViewportType].OffsetLow;
		MidOffset += ViewOffset_ViewportAdjustments[CurrentViewportType].OffsetMid;
		HighOffset += ViewOffset_ViewportAdjustments[CurrentViewportType].OffsetHigh;
	}
	float const Pitch = GetViewPitch(ViewedPawn, ViewRotation);
	if (bSmoothViewOffsetPitchChanges)
	{
		FInterpCurveVector Curve;
		Curve.AddPoint(ThirdPersonCam->PlayerCamera->ViewPitchMin, HighOffset);
		Curve.AddPoint(0.f, MidOffset);
		Curve.AddPoint(ThirdPersonCam->PlayerCamera->ViewPitchMax, LowOffset);
		Curve.Points[0].InterpMode = CIM_CurveAuto;
		Curve.Points[1].InterpMode = CIM_CurveAuto;
		Curve.Points[2].InterpMode = CIM_CurveAuto;
		Curve.AutoSetTangents();
		out_Offset = Curve.Eval(Pitch, MidOffset);
	}
	else
	{
		if (Pitch >= 0.f)
		{
			float Pct = Pitch / ThirdPersonCam->PlayerCamera->ViewPitchMax;
			out_Offset = FMath::Lerp<FVector, float>(MidOffset, LowOffset, Pct);
		}
		else
		{
			float Pct = Pitch / ThirdPersonCam->PlayerCamera->ViewPitchMin;
			out_Offset = FMath::Lerp<FVector, float>(MidOffset, HighOffset, Pct);
		}
	}
	FVector const OffsetPreAdjustment = out_Offset;
	out_Offset = AdjustViewOffset(ViewedPawn, out_Offset);
	FVector const AdjustmentDelta = out_Offset - OffsetPreAdjustment;
	FVector NewDelta = (ThirdPersonCam && !ThirdPersonCam->bResetCameraInterpolation)
		? FMath::VInterpTo(ThirdPersonCam->LastOffsetAdjustment, AdjustmentDelta, DeltaTime, OffsetAdjustmentInterpSpeed)
		: AdjustmentDelta;
	if (ThirdPersonCam)
	{
		ThirdPersonCam->LastOffsetAdjustment = NewDelta;
	}
	out_Offset = OffsetPreAdjustment + NewDelta;
	return out_Offset;
}

float UGameThirdPersonCameraMode::GetViewOffsetInterpSpeed(APawn* ViewedPawn, float DeltaTime)
{
	float Result = 0.f;
	if (ViewedPawn)
	{
		float BlendingTime = GetBlendTime(ViewedPawn);
		if (BlendingTime > 0.f)
		{
			Result = 1.f / BlendingTime;
		}
	}
	if (bInterpViewOffsetOnlyForCamTransition && Result > 0.f)
	{
		ViewOffsetInterp += Result * DeltaTime;
		ViewOffsetInterp = FMath::Min<float>(ViewOffsetInterp, 10000.f);
		return ViewOffsetInterp;
	}
	return Result;
}

FRotator UGameThirdPersonCameraMode::GetViewOffsetRotBase(APawn* ViewedPawn, const FTViewTarget& VT)
{
	return VT.POV.Rotation;
}

void UGameThirdPersonCameraMode::GetBaseViewOffsets(APawn* ViewedPawn, TEnumAsByte<ECameraViewportTypes> ViewportConfig, float DeltaTime, FVector& out_Low, FVector& out_Mid, FVector& out_High)
{
	FVector StrafeOffset(FVector::ZeroVector);
	FVector RunOffset(FVector::ZeroVector);
	if (ViewedPawn)
	{
		float VelMag = ViewedPawn->GetVelocity().Size();
		if (VelMag > 0.f)
		{
			FVector X, Y, Z;
			FRotationMatrix(ViewedPawn->GetActorRotation()).GetUnitAxes(X, Y, Z);
			FVector NormalVel = ViewedPawn->GetVelocity() / VelMag;
			if (StrafeOffsetScalingThreshold > 0.f)
			{
				float YDot = Y | NormalVel;
				if (YDot < 0.f)
				{
					StrafeOffset = StrafeLeftAdjustment * -YDot;
				}
				else
				{
					StrafeOffset = StrafeRightAdjustment * YDot;
				}

				StrafeOffset *= FMath::Clamp(VelMag / StrafeOffsetScalingThreshold, 0.f, 1.f);
			}
			if (RunOffsetScalingThreshold > 0.f)
			{
				float XDot = X | NormalVel;
				if (XDot < 0.f)
				{
					RunOffset = RunBackAdjustment * -XDot;
				}
				else
				{
					RunOffset = RunFwdAdjustment * XDot;
				}
				RunOffset *= FMath::Clamp(VelMag / RunOffsetScalingThreshold, 0.f, 1.f);
			}
		}
		float Speed = StrafeOffset.IsZero() ? StrafeOffsetInterpSpeedOut : StrafeOffsetInterpSpeedIn;
		StrafeOffset = FMath::VInterpTo(LastStrafeOffset, StrafeOffset, DeltaTime, Speed);
		LastStrafeOffset = StrafeOffset;

		Speed = RunOffset.IsZero() ? RunOffsetInterpSpeedOut : RunOffsetInterpSpeedIn;
		RunOffset = FMath::VInterpTo(LastRunOffset, RunOffset, DeltaTime, Speed);
		LastRunOffset = RunOffset;

		FRotator CamRot(FRotator::ZeroRotator);
		FVector UnusedVec(FVector::ZeroVector);

		if (ViewedPawn->GetController())
		{
			ViewedPawn->GetController()->GetPlayerViewPoint(UnusedVec, CamRot);
		}
		else
		{
			CamRot = ViewedPawn->GetActorRotation();
		}

		FVector TotalOffset = StrafeOffset + RunOffset;
		TotalOffset = ::TransformWorldToLocal(TotalOffset, ViewedPawn->GetActorRotation());
		TotalOffset = ::TransformLocalToWorld(TotalOffset, CamRot);
		out_Low = ViewOffset.OffsetLow + TotalOffset;
		out_Mid = ViewOffset.OffsetMid + TotalOffset;
		out_High = ViewOffset.OffsetHigh + TotalOffset;
	}
}

bool UGameThirdPersonCameraMode::UseDirectLookMode(APawn* CameraTarget)
{
	return bDirectLook;
}

bool UGameThirdPersonCameraMode::LockedToViewTarget(APawn* CameraTarget)
{
	return bLockedToViewTarget;
}

bool UGameThirdPersonCameraMode::ShouldFollowTarget(APawn* CameraTarget, float& PitchInterpSpeed, float& YawInterpSpeed, float& RollInterpSpeed)
{
	return (bLockedToViewTarget) ? false : bFollowTarget;
}

FVector UGameThirdPersonCameraMode::GetTargetRelativeOriginOffset(APawn* TargetPawn)
{
	return TargetRelativeCameraOriginOffset;
}

void UGameThirdPersonCameraMode::GetCameraOrigin(APawn* TargetPawn, FVector& OriginLoc, FRotator& OriginRot)
{
	if (TargetPawn && (ThirdPersonCam && ThirdPersonCam->bResetCameraInterpolation) || LockedToViewTarget(TargetPawn))
	{
		OriginRot = TargetPawn->GetViewRotation();
	}
	else
	{
		OriginRot = ThirdPersonCam->PlayerCamera->GetCameraRotation();
	}
	OriginLoc = TargetPawn->GetPawnViewLocation();
	OriginLoc += ::TransformLocalToWorld(GetTargetRelativeOriginOffset(TargetPawn), TargetPawn->GetActorRotation());
}

void UGameThirdPersonCameraMode::InterpolateCameraOrigin(APawn* TargetPawn, float DeltaTime, FVector& out_ActualCameraOrigin, FVector const& IdealCameraOrigin, FRotator& out_ActualCameraOriginRot, FRotator const& IdealCameraOriginRot)
{
	if (ThirdPersonCam && ThirdPersonCam->bResetCameraInterpolation)
	{
		out_ActualCameraOrigin = IdealCameraOrigin;
	}
	else
	{
		out_ActualCameraOrigin = InterpolateCameraOriginLoc(TargetPawn, TargetPawn->GetActorRotation(), ThirdPersonCam->LastActualCameraOriginLoc, IdealCameraOrigin, DeltaTime);
	}
	if (ThirdPersonCam && ThirdPersonCam->bResetCameraInterpolation)
	{
		out_ActualCameraOriginRot = IdealCameraOriginRot;
	}
	else
	{
		out_ActualCameraOriginRot = InterpolateCameraOriginRot(TargetPawn, ThirdPersonCam->LastActualCameraOriginRot, IdealCameraOriginRot, DeltaTime);
	}
}

float UGameThirdPersonCameraMode::GetBlendTime(APawn* Pawn)
{
	return BlendTime;
}

float UGameThirdPersonCameraMode::GetFOVBlendTime(APawn* Pawn)
{
	return BlendTime;
}

FVector UGameThirdPersonCameraMode::InterpolateCameraOriginLoc(APawn* TargetPawn, FRotator const& CameraTargetRot, FVector const& LastLoc, FVector const& IdealLoc, float DeltaTime)
{
	if (!bInterpLocation)
	{
		return IdealLoc;
	}
	else
	{
		if (bUsePerAxisOriginLocInterp)
		{
			FVector OriginLoc(FVector::ZeroVector);
			const FRotationMatrix RotMatrix(CameraTargetRot);
			const FMatrix RotMatrixInverse = RotMatrix.Inverse();
			const FVector LastLocInCameraLocal = RotMatrixInverse.TransformPosition(LastLoc);
			const FVector IdealLocInCameraLocal = RotMatrixInverse.TransformPosition(IdealLoc);
			FVector OriginLocInCameraLocal(FVector::ZeroVector);
			OriginLocInCameraLocal.X = FMath::FInterpTo(LastLocInCameraLocal.X, IdealLocInCameraLocal.X, DeltaTime, PerAxisOriginLocInterpSpeed.X);
			OriginLocInCameraLocal.Y = FMath::FInterpTo(LastLocInCameraLocal.Y, IdealLocInCameraLocal.Y, DeltaTime, PerAxisOriginLocInterpSpeed.Y);
			OriginLocInCameraLocal.Z = FMath::FInterpTo(LastLocInCameraLocal.Z, IdealLocInCameraLocal.Z, DeltaTime, PerAxisOriginLocInterpSpeed.Z);
			OriginLoc = RotMatrix.TransformPosition(OriginLocInCameraLocal);
			return OriginLoc;
		}
		else
		{
			return FMath::VInterpTo(LastLoc, IdealLoc, DeltaTime, OriginLocInterpSpeed);
		}
	}
}

FRotator UGameThirdPersonCameraMode::InterpolateCameraOriginRot(APawn* TargetPawn, FRotator const& LastRot, FRotator const& IdealRot, float DeltaTime)
{
	if (!bInterpRotation)
	{
		return IdealRot;
	}
	else
	{
		return FMath::RInterpTo(LastRot, IdealRot, DeltaTime, OriginRotInterpSpeed);
	}
}

FVector UGameThirdPersonCameraMode::ApplyViewOffset(APawn* ViewedPawn, const FVector& CameraOrigin, const FVector& ActualViewOffset, const FVector& DeltaViewOffset, const FTViewTarget& OutVT)
{
	if (bApplyDeltaViewOffset)
	{
		return CameraOrigin + ::TransformLocalToWorld(DeltaViewOffset, GetViewOffsetRotBase(ViewedPawn, OutVT));
	}
	else
	{
		return CameraOrigin + ::TransformLocalToWorld(ActualViewOffset, GetViewOffsetRotBase(ViewedPawn, OutVT));
	}
}

float UGameThirdPersonCameraMode::GetViewPitch(APawn* TargetPawn, FRotator const& ViewRotation) const
{
	return FRotator::NormalizeAxis(ViewRotation.Pitch);
}

float UGameThirdPersonCameraMode::GetDesiredFOV(APawn* ViewedPawn)
{
	return FOVAngle;
}


void UGameThirdPersonCameraMode::OnBecomeActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* PrevMode)
{
	if (BlendTime > 0.f)
	{
		ViewOffsetInterp = 1.f / BlendTime;
	}
	else
	{
		ViewOffsetInterp = 0.f;
	}
}

void UGameThirdPersonCameraMode::OnBecomeInActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* NewMode)
{
	//
}

void UGameThirdPersonCameraMode::Init()
{

}

void UGameThirdPersonCameraMode::ProcessViewRotation(float DeltaTime, AActor* ViewTarget, FRotator& out_ViewRotation, FRotator& out_DeltaRot)
{

}

void UGameThirdPersonCameraMode::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.SetDrawColor(FColor::Red);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("GameThirdPersonCameraMode %s"), *GetNameSafe(this)));
}




/**
* UGameThirdPersonCameraMode_Default
**/
UGameThirdPersonCameraMode_Default::UGameThirdPersonCameraMode_Default()
{
	TemporaryOriginRotInterpSpeed = 12.f;

	WorstLocOffset = FVector(-8.f, 1.f, 95.f);
	WorstLocAimingZOffset = -10.f;
	bValidateWorstLoc = false;

	ViewOffset.OffsetHigh = FVector(-128.f, 56.f, 40.f);
	ViewOffset.OffsetLow = FVector(-160.f, 48.f, 56.f);
	ViewOffset.OffsetMid = FVector(-160.f, 48.f, 16.f);

	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_HorizSplit].OffsetHigh = FVector(0, 0, -12);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_HorizSplit].OffsetLow = FVector(0, 0, -12);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_HorizSplit].OffsetMid = FVector(0, 0, -12);

	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_VertSplit].OffsetHigh = FVector(0, -20, 0);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_VertSplit].OffsetLow = FVector(0, -20, 0);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_VertSplit].OffsetMid = FVector(0, -20, 0);

	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_Full].OffsetHigh = FVector(0, 0, 17);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_Full].OffsetLow = FVector(0, 0, 17);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_Full].OffsetMid = FVector(0, 0, 17);

	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_HorizSplit].OffsetHigh = FVector(0, 0, -15);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_HorizSplit].OffsetLow = FVector(0, 0, -15);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_HorizSplit].OffsetMid = FVector(0, 0, -15);

	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_VertSplit].OffsetHigh = FVector::ZeroVector;
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_VertSplit].OffsetLow = FVector::ZeroVector;
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_VertSplit].OffsetMid = FVector::ZeroVector;

	StrafeLeftAdjustment = FVector(0, -15.f, 0);
	StrafeRightAdjustment = FVector(0, 15.f, 0);
	StrafeOffsetScalingThreshold = 200;

	RunFwdAdjustment = FVector(20.f, 0, 0);
	RunBackAdjustment = FVector(-30.f, 0, 0);
	RunOffsetScalingThreshold = 200;

	BlendTime = 0.25;

	FOVAngle = 70.f;
}

void UGameThirdPersonCameraMode_Default::Init()
{

}

void UGameThirdPersonCameraMode_Default::GetCameraOrigin(APawn* TargetPawn, FVector& OriginLoc, FRotator& OriginRot)
{
	Super::GetCameraOrigin(TargetPawn, OriginLoc, OriginRot);
	if (TargetPawn && ThirdPersonCam && bTemporaryOriginRotInterp)
	{
		FRotator BaseOriginRot = OriginRot;
		OriginRot = FMath::RInterpTo(ThirdPersonCam->LastActualCameraOriginRot, BaseOriginRot, TemporaryOriginRotInterpSpeed, TargetPawn->GetWorld()->GetDeltaSeconds());
		if (OriginRot == BaseOriginRot)
		{
			bTemporaryOriginRotInterp = false;
		}
	}
}

void UGameThirdPersonCameraMode_Default::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.SetDrawColor(FColor::Green);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("UGameThirdPersonCameraMode_Default %s"), *GetNameSafe(this)));
}


/**
* UGameThirdPersonCamera
**/
UGameThirdPersonCamera::UGameThirdPersonCamera()
{
	PenetrationBlendOutTime = 0.15f;
	PenetrationBlendInTime = 0.10f;
	PenetrationBlockedPct = 1.0f;
	PenetrationExtentScale = 1.0f;
	WorstLocPenetrationExtentScale = 1.0f;
	WorstLocInterpSpeed = 8;
	bResetCameraInterpolation = true;
	OriginOffsetInterpSpeed = 8;

	bDoingACameraTurn = false;

	DirectLookInterpSpeed = 6.f;

	PenetrationAvoidanceFeelers.Empty();
	PenetrationAvoidanceFeelers.AddZeroed(7);

	PenetrationAvoidanceFeelers[0].AdjustmentRot = FRotator::ZeroRotator;
	PenetrationAvoidanceFeelers[0].WorldWeight = 1.f;
	PenetrationAvoidanceFeelers[0].PawnWeight = 1.f;
	PenetrationAvoidanceFeelers[0].Extent = FVector(14.f, 14.f, 14.f);
	PenetrationAvoidanceFeelers[1].AdjustmentRot = FRotator(0, 17.f, 0);
	PenetrationAvoidanceFeelers[1].WorldWeight = 0.75f;
	PenetrationAvoidanceFeelers[1].PawnWeight = 0.75f;
	PenetrationAvoidanceFeelers[1].Extent = FVector::ZeroVector;
	PenetrationAvoidanceFeelers[1].TraceInterval = 3;
	PenetrationAvoidanceFeelers[2].AdjustmentRot = FRotator(0, -17.f, 0);
	PenetrationAvoidanceFeelers[2].WorldWeight = 0.75f;
	PenetrationAvoidanceFeelers[2].PawnWeight = 0.75f;
	PenetrationAvoidanceFeelers[2].Extent = FVector::ZeroVector;
	PenetrationAvoidanceFeelers[2].TraceInterval = 3;
	PenetrationAvoidanceFeelers[3].AdjustmentRot = FRotator(0, 34.f, 0);
	PenetrationAvoidanceFeelers[3].WorldWeight = 0.5f;
	PenetrationAvoidanceFeelers[3].PawnWeight = 0.5f;
	PenetrationAvoidanceFeelers[3].Extent = FVector::ZeroVector;
	PenetrationAvoidanceFeelers[3].TraceInterval = 5;
	PenetrationAvoidanceFeelers[4].AdjustmentRot = FRotator(0, -34.f, 0);
	PenetrationAvoidanceFeelers[4].WorldWeight = 0.5f;
	PenetrationAvoidanceFeelers[4].PawnWeight = 0.5f;
	PenetrationAvoidanceFeelers[4].Extent = FVector::ZeroVector;
	PenetrationAvoidanceFeelers[4].TraceInterval = 5;
	PenetrationAvoidanceFeelers[5].AdjustmentRot = FRotator(20.f, 0, 0);
	PenetrationAvoidanceFeelers[5].WorldWeight = 1.f;
	PenetrationAvoidanceFeelers[5].PawnWeight = 1.f;
	PenetrationAvoidanceFeelers[5].Extent = FVector::ZeroVector;
	PenetrationAvoidanceFeelers[5].TraceInterval = 5;
	PenetrationAvoidanceFeelers[6].AdjustmentRot = FRotator(-20.f, 0, 0);
	PenetrationAvoidanceFeelers[6].WorldWeight = 0.5f;
	PenetrationAvoidanceFeelers[6].PawnWeight = 0.5f;
	PenetrationAvoidanceFeelers[6].Extent = FVector::ZeroVector;
	PenetrationAvoidanceFeelers[6].TraceInterval = 4;
	ThirdPersonCamDefaultClass = UGameThirdPersonCameraMode_Default::StaticClass();
	bDrawCameraPawnDebug = false;
	bDrawDebug = false;
}

void UGameThirdPersonCamera::InterpolateCameraOrigin(APawn* TargetPawn, float DeltaTime, FVector& out_ActualCameraOrigin, FVector const& IdealCameraOrigin, FRotator& out_ActualCameraOriginRot, FRotator const& IdealCameraOriginRot)
{
	if (CurrentCamMode)
	{
		CurrentCamMode->InterpolateCameraOrigin(TargetPawn, DeltaTime, out_ActualCameraOrigin, IdealCameraOrigin, out_ActualCameraOriginRot, IdealCameraOriginRot);
	}

}



void UGameThirdPersonCamera::PreventCameraPenetration(APawn* P, class ASandboxCameraManager* CameraActor, const FVector& WorstLocation, FVector& DesiredLocation, float DeltaTime, float& DistBlockedPct, float CameraExtentScale, bool bSingleRayOnly /*= false*/)
{
	/** from player camera manager
	FCollisionQueryParams BoxParams(SCENE_QUERY_STAT(FreeCam), false, this);
	BoxParams.AddIgnoredActor(OutVT.Target);
	FHitResult Result;

	GetWorld()->SweepSingleByChannel(Result, Loc, Pos, FQuat::Identity, ECC_Camera, FCollisionShape::MakeBox(FVector(12.f)), BoxParams);
	OutVT.POV.Location = !Result.bBlockingHit ? Pos : Result.Location;
	OutVT.POV.Rotation = Rotator;
	**/
	
	const UWorld* World = CameraActor->GetWorld();
	if (!World)
		return;
	
	float HardBlockedPct = DistBlockedPct;
	float SoftBlockedPct = DistBlockedPct;
	FVector BaseRay = DesiredLocation - WorstLocation;
	FRotationMatrix BaseRayMatrix(BaseRay.Rotation());
	FVector BaseRayLocalUp(FVector::ZeroVector);
	FVector BaseRayLocalFwd(FVector::ZeroVector);
	FVector BaseRayLocalRight(FVector::ZeroVector);
	BaseRayMatrix.GetScaledAxes(BaseRayLocalFwd, BaseRayLocalRight, BaseRayLocalUp);
	float CheckDist = BaseRay.Size();
	float DistBlockedPctThisFrame = 1.f;
	//FlushPersistentDebugLines(P->GetWorld());
	const int32 NumRaysToShoot = (bSingleRayOnly) ? FMath::Min<int32>(1, PenetrationAvoidanceFeelers.Num()) : PenetrationAvoidanceFeelers.Num();
	for (int32 RayIdx = 0; RayIdx < NumRaysToShoot; ++RayIdx)
	{
		FPenetrationAvoidanceFeeler& Feeler = PenetrationAvoidanceFeelers[RayIdx];
		if (Feeler.FramesUntilNextTrace <= 0)
		{
			FVector RayTarget(FVector::ZeroVector);
			{
				FVector RotatedRay = BaseRay.RotateAngleAxis(Feeler.AdjustmentRot.Yaw, BaseRayLocalUp);
				RotatedRay = RotatedRay.RotateAngleAxis(Feeler.AdjustmentRot.Pitch, BaseRayLocalRight);
				RayTarget = WorstLocation + RotatedRay;
			}
			TArray<FHitResult> CameraHitResults;
			CameraHitResults.Init(FHitResult(EForceInit::ForceInit), Feeler.TraceInterval);
			static const FName CameraTrace(TEXT("PreventCameraPenetration"));
			ECollisionChannel TraceChannel = (Feeler.PawnWeight > 0.f) ? ECollisionChannel::ECC_Pawn : ECollisionChannel::ECC_Visibility;
			FCollisionQueryParams QueryParams(CameraTrace, ShouldDoPerPolyPenetrationTests(P), CameraActor);
			
			const bool bHit = World->LineTraceMultiByChannel(CameraHitResults, WorstLocation, RayTarget, TraceChannel, QueryParams);
			Feeler.FramesUntilNextTrace = Feeler.TraceInterval;
			for (int32 HitIdx = 0; HitIdx < CameraHitResults.Num(); ++HitIdx)
			{
				FHitResult const& Hit = CameraHitResults[HitIdx];
				if ((Hit.GetActor() != nullptr) && !ShouldIgnorePenetrationHit(Hit, P))
				{
					float const Weight = Hit.GetActor()->GetInstigator() ? Feeler.PawnWeight : Feeler.WorldWeight;
					float NewBlockPct = Hit.Time;
					NewBlockPct += (1.f - NewBlockPct) * (1.f - Weight);
					DistBlockedPctThisFrame = FMath::Min<float>(NewBlockPct, DistBlockedPctThisFrame);

					// This feeler got a hit, so do another trace next frame
					Feeler.FramesUntilNextTrace = 0;
				}
			}

			if (RayIdx == 0)
			{
				// don't interpolate toward this one, snap to it
				// assumes ray 0 is the center/main ray 
				HardBlockedPct = DistBlockedPctThisFrame;
			}
			else
			{
				SoftBlockedPct = DistBlockedPctThisFrame;
			}
		}
		else
		{
			--Feeler.FramesUntilNextTrace;
		}
	}

	if (DistBlockedPct < DistBlockedPctThisFrame)
	{
		// interpolate smoothly out
		if (PenetrationBlendOutTime > DeltaTime)
		{
			DistBlockedPct = DistBlockedPct + DeltaTime / PenetrationBlendOutTime * (DistBlockedPctThisFrame - DistBlockedPct);
		}
		else
		{
			DistBlockedPct = DistBlockedPctThisFrame;
		}
	}
	else
	{
		if (DistBlockedPct > HardBlockedPct)
		{
			DistBlockedPct = HardBlockedPct;
		}
		else if (DistBlockedPct > SoftBlockedPct)
		{
			// interpolate smoothly in
			if (PenetrationBlendInTime > DeltaTime)
			{
				DistBlockedPct = DistBlockedPct - DeltaTime / PenetrationBlendInTime * (DistBlockedPct - SoftBlockedPct);
			}
			else
			{
				DistBlockedPct = SoftBlockedPct;
			}
		}
	}
	DistBlockedPct = FMath::Clamp<float>(DistBlockedPct, 0.f, 1.f);
	if (DistBlockedPct < KINDA_SMALL_NUMBER)
	{
		DistBlockedPct = 0.f;
	}
	if (DistBlockedPct < 1.f)
	{
		DesiredLocation = WorstLocation + (DesiredLocation - WorstLocation) * DistBlockedPct;
	}
}

void UGameThirdPersonCamera::UpdateForMovingBase(UPrimitiveComponent* BaseActor)
{
	//nothing
}

FMatrix UGameThirdPersonCamera::GetWorstCaseLocTransform(APawn* P) const
{
	return FRotationTranslationMatrix(P->GetActorRotation(), P->GetActorLocation());
}

bool UGameThirdPersonCamera::ShouldIgnorePenetrationHit(FHitResult Hit, APawn* TargetPawn) const
{
	APawn* HitPawn = Hit.GetActor()->GetInstigator();
	if (HitPawn)
	{
		if (TargetPawn && (HitPawn == TargetPawn))
		{
			return true;
		}
	}

	return false;
}

bool UGameThirdPersonCamera::ShouldDoPredictavePenetrationAvoidance(APawn* TargetPawn) const
{
	return CurrentCamMode->bDoPredictiveAvoidance;
}

void UGameThirdPersonCamera::HandlePawnPenetration(FTViewTarget& OutVT)
{
	// nothing
}

UGameThirdPersonCameraMode* UGameThirdPersonCamera::CreateCameraMode(TSubclassOf<UGameThirdPersonCameraMode> ModeClass)
{
	UGameThirdPersonCameraMode* NewCameraMode = NewObject<UGameThirdPersonCameraMode>(this, ModeClass);
	if (NewCameraMode)
	{
		NewCameraMode->ThirdPersonCam = this;
		NewCameraMode->Init();
	}
	return NewCameraMode;
}

void UGameThirdPersonCamera::Reset()
{
	bResetCameraInterpolation = true;
}

void UGameThirdPersonCamera::PlayerUpdateCamera(APawn* P, class ASandboxCameraManager* CameraActor, float DeltaTime, struct FTViewTarget& OutVT)
{
	ACharacter* const GP = Cast<ACharacter>(P);

#if WITH_EDITOR
	bDebugChangedCameraMode = false;
#endif
	
	UpdateForMovingBase(GP->GetMovementBase());
	FVector		IdealCameraOriginLoc;
	FRotator	IdealCameraOriginRot;
	CurrentCamMode->GetCameraOrigin(P, IdealCameraOriginLoc, IdealCameraOriginRot);
	FVector ActualCameraOriginLoc;
	FRotator ActualCameraOriginRot;
	InterpolateCameraOrigin(P, DeltaTime, ActualCameraOriginLoc, IdealCameraOriginLoc, ActualCameraOriginRot, IdealCameraOriginRot);
	LastIdealCameraOriginLoc = IdealCameraOriginLoc;
	LastIdealCameraOriginRot = IdealCameraOriginRot;
	LastActualCameraOriginLoc = ActualCameraOriginLoc;
	LastActualCameraOriginRot = ActualCameraOriginRot;
	ActualCameraOriginLoc += GetPostInterpCameraOriginLocationOffset(P);
	ActualCameraOriginRot += GetPostInterpCameraOriginRotationOffset(P);
	FVector IdealViewOffset = CurrentCamMode->GetViewOffset(P, DeltaTime, ActualCameraOriginLoc, ActualCameraOriginRot);
	OutVT.POV.FOV = GetDesiredFOV(P);
	OutVT.POV.Rotation = ActualCameraOriginRot;
	if (bDoingACameraTurn)
	{
		TurnCurTime += DeltaTime;
		float TurnInterpPct = (TurnCurTime - TurnDelay) / TurnTotalTime;
		TurnInterpPct = FMath::Clamp<float>(TurnInterpPct, 0.f, 1.f);
		if (TurnInterpPct == 1.f)
		{
			EndTurn();
		}
		float TurnAngle = FMath::InterpEaseInOut(TurnStartAngle, TurnEndAngle, TurnInterpPct, 2.f);
		OutVT.POV.Rotation.Yaw += TurnAngle;
		LastPostCamTurnYaw = OutVT.POV.Rotation.Yaw;
	}
	float FOVBlendTime = CurrentCamMode->GetFOVBlendTime(P);
	if (!bResetCameraInterpolation && FOVBlendTime > 0.f)
	{
		float InterpSpeed = 1.f / FOVBlendTime;
		OutVT.POV.FOV = FMath::FInterpTo(LastCamFOV, OutVT.POV.FOV, DeltaTime, InterpSpeed);
	}
	LastCamFOV = OutVT.POV.FOV;
	FVector ActualViewOffset(FVector::ZeroVector);
	FVector DeltaViewOffset(FVector::ZeroVector);
	{
		float InterpSpeed = CurrentCamMode->GetViewOffsetInterpSpeed(P, DeltaTime);
		if (!bResetCameraInterpolation && InterpSpeed > 0.f)
		{
			ActualViewOffset = FMath::VInterpTo(LastViewOffset, IdealViewOffset, DeltaTime, InterpSpeed);
		}
		else
		{
			ActualViewOffset = IdealViewOffset;
		}

		DeltaViewOffset = (ActualViewOffset - LastViewOffset);
		LastViewOffset = ActualViewOffset;
	}

	if (!bDoingACameraTurn)
	{
		bool bDirectLook = CurrentCamMode->UseDirectLookMode(P);
		if (bDirectLook)
		{
			bool const bMoving = (P->GetVelocity().SizeSquared() > 50.f) ? true : false;
			FRotator BaseRot = (bMoving) ? P->GetVelocity().Rotation() : P->GetActorRotation();
			if ((DirectLookYaw != 0.f) || bDoingDirectLook)
			{
				BaseRot.Yaw = FRotator::NormalizeAxis(BaseRot.Yaw + DirectLookYaw);
				OutVT.POV.Rotation = FMath::RInterpTo(OutVT.POV.Rotation, BaseRot, DeltaTime, DirectLookInterpSpeed);
				if (DirectLookYaw == 0.f)
				{
					int const StopDirectLookThresh = bMoving ? 1000 : 50;
					if (FMath::Abs(OutVT.POV.Rotation.Yaw - BaseRot.Yaw) < StopDirectLookThresh)
					{
						bDoingDirectLook = false;
					}
				}
				else
				{
					bDoingDirectLook = true;
				}
			}
		}
		bool bLockedToViewTarget = CurrentCamMode->LockedToViewTarget(P);
		if (!bLockedToViewTarget)
		{
			float PitchInterpSpeed, YawInterpSpeed, RollInterpSpeed;
			if ((P->GetVelocity().SizeSquared() > 50.f) &&
				CurrentCamMode->ShouldFollowTarget(P, PitchInterpSpeed, YawInterpSpeed, RollInterpSpeed))
			{
				float Scale;
				if (CurrentCamMode->FollowingCameraVelThreshold > 0.f)
				{
					Scale = FMath::Min<float>(1.0f, (P->GetVelocity().Size() / CurrentCamMode->FollowingCameraVelThreshold));
				}
				else
				{
					Scale = 1.f;
				}

				PitchInterpSpeed *= Scale;
				YawInterpSpeed *= Scale;
				RollInterpSpeed *= Scale;
				const FRotator BaseRot = P->GetVelocity().Rotation();
				OutVT.POV.Rotation = RInterpToWithPerAxisSpeeds(OutVT.POV.Rotation, BaseRot, DeltaTime, PitchInterpSpeed, YawInterpSpeed, RollInterpSpeed);
			}
		}
	}

	FVector	DesiredCamLoc = CurrentCamMode->ApplyViewOffset(P, ActualCameraOriginLoc, ActualViewOffset, DeltaViewOffset, OutVT);
	OutVT.POV.Location = DesiredCamLoc;
	LastPreModifierCameraLoc = OutVT.POV.Location;
	LastPreModifierCameraRot = OutVT.POV.Rotation;
	HandleCameraSafeZone(OutVT.POV.Location, OutVT.POV.Rotation, DeltaTime);
	if (PlayerCamera)
	{
		float FOVBeforePostProcess = OutVT.POV.FOV;
		PlayerCamera->ApplyCameraModifiers(DeltaTime, OutVT.POV);
		if (CurrentCamMode->bNoFOVPostProcess)
		{
			OutVT.POV.FOV = FOVBeforePostProcess;
		}
	}
	FVector IdealWorstLocationLocal = CurrentCamMode->GetCameraWorstCaseLoc(P, OutVT);
	FMatrix CamSpaceToWorld = GetWorstCaseLocTransform(P);
	IdealWorstLocationLocal = CamSpaceToWorld.InverseTransformPosition(IdealWorstLocationLocal);
	FVector WorstLocation = !bResetCameraInterpolation ?
		FMath::VInterpTo(LastWorstLocationLocal, IdealWorstLocationLocal, DeltaTime, WorstLocInterpSpeed)
		: IdealWorstLocationLocal;
	LastWorstLocationLocal = WorstLocation;
	WorstLocation = CamSpaceToWorld.TransformPosition(WorstLocation);

	const UWorld* CurrentWorld = CameraActor->GetWorld();
	//	draw point worst location
#if WITH_EDITOR && DEBUGCAMERA_PENETRATION
	//FlushPersistentDebugLines(CurrentWorld);
	//DrawDebugSphere(CurrentWorld, WorstLocation, 5.f, 8, FColor::Orange);
#endif

	if (CurrentCamMode->bValidateWorstLoc)
	{
		PreventCameraPenetration(P, CameraActor, P->GetActorLocation(), WorstLocation, DeltaTime, WorstLocBlockedPct, WorstLocPenetrationExtentScale, true);
	}
	else
	{
		WorstLocBlockedPct = 1.f;
	}

#if WITH_EDITOR && DEBUGCAMERA_PENETRATION
	//DrawDebugLine(CurrentWorld, WorstLocation, OutVT.POV.Location, FColor::Magenta);
	//DrawDebugSphere(CurrentWorld, WorstLocation, 8.f, 12, FColor::Yellow);
#endif

	// adjust final desired camera location, to again, prevent any penetration
	if (!CurrentCamMode->bSkipCameraCollision)
	{
		const bool bSingleRayPenetrationCheck = !ShouldDoPredictavePenetrationAvoidance(P);
		PreventCameraPenetration(P, CameraActor, WorstLocation, OutVT.POV.Location, DeltaTime, PenetrationBlockedPct, PenetrationExtentScale, bSingleRayPenetrationCheck);
	}

	HandlePawnPenetration(OutVT);

	//	debug camera
#if WITH_EDITOR	&& DEBUGCAMERA_PENETRATION

	if (bDrawCameraPawnDebug)
	{
		DrawDebugBox(CurrentWorld, OutVT.POV.Location, FVector(5.f), FColor::White);
		DrawDebugLine(CurrentWorld, OutVT.POV.Location, OutVT.POV.Location + OutVT.POV.Rotation.Vector()* 128.f, FColor::White);
		GEngine->AddOnScreenDebugMessage(1, 1.0f, FColor::White, FString::Printf(TEXT("WhiteBox: Final Camera Location, Red Line : Final Camera Rotation")));
		GEngine->AddOnScreenDebugMessage(2, 1.0f, FColor::Green, FString::Printf(TEXT("GreenLine: Matinee Camera Path, Yellow Line: Matinee Camera Rotation")));
		GEngine->AddOnScreenDebugMessage(3, 1.0f, FColor::Yellow, FString::Printf(TEXT("Yellow Box: Player's initial Location")));
		GEngine->AddOnScreenDebugMessage(4, 1.0f, FColor::Magenta, FString::Printf(TEXT("Coordinates: Changes of players location with rotation")));
		GEngine->AddOnScreenDebugMessage(5, 1.0f, FColor::Blue, FString::Printf(TEXT("Try \"ToggleDebugCamera\" to examine closer")));
		GEngine->AddOnScreenDebugMessage(6, 1.0f, FColor::Red, FString::Printf(TEXT("To remove lines, toggle \"DebugCameraAnims\"")));

		// if current active one is available
		if (PlayerCamera->ActiveAnims.Num() > 0)
		{
			DrawDebugBox(CurrentWorld, OutVT.POV.Location, FVector(5.f), FColor::Yellow);
			DrawDebugLine(CurrentWorld, OutVT.POV.Location, OutVT.POV.Location + OutVT.POV.Rotation.Vector() * 100.f, FColor::Red);
		}
	}

	if (bDrawDebug)
	{
		GEngine->AddOnScreenDebugMessage(7, 1.0f, FColor::White, FString::Printf(TEXT("OutVT.POV.Location: %s"), *OutVT.POV.Location.ToCompactString()));
		GEngine->AddOnScreenDebugMessage(8, 1.0f, FColor::White, FString::Printf(TEXT("DesiredCamLoc: %f %f %f"), *DesiredCamLoc.ToCompactString()));
		GEngine->AddOnScreenDebugMessage(9, 1.0f, FColor::White, FString::Printf(TEXT("OutVT.POV.FOV: %.3f"), OutVT.POV.FOV));

		if (GP && GP->GetMesh())
		{
			FVector RootBoneOffset = OutVT.POV.Location - GP->GetMesh()->GetBoneMatrix(0).GetOrigin();
			GEngine->AddOnScreenDebugMessage(9, 1.0f, FColor::White, FString::Printf(TEXT("Offset from root bone: %s"), *RootBoneOffset.ToCompactString()));
		}

		GEngine->AddOnScreenDebugMessage(10, 1.0f, FColor::White, FString::Printf(TEXT("IdealCameraOrigin: %s"), *IdealCameraOriginLoc.ToCompactString()));
		GEngine->AddOnScreenDebugMessage(11, 1.0f, FColor::White, FString::Printf(TEXT("ActualCameraOrigin: %s"), *ActualCameraOriginLoc.ToCompactString()));

		DrawDebugBox(CurrentWorld, IdealCameraOriginLoc, FVector(8.f, 8.f, 8.f), FColor::Green);
		DrawDebugBox(CurrentWorld, ActualCameraOriginLoc, FVector(7.f, 7.f, 7.f), FColor(128, 255, 128));
		DrawDebugBox(CurrentWorld, DesiredCamLoc, FVector(15.f, 15.f, 15.f), FColor::Blue);
		DrawDebugBox(CurrentWorld, OutVT.POV.Location, FVector(14.f, 14.f, 14.f), FColor(128, 128, 255));
		DrawDebugBox(CurrentWorld, WorstLocation, FVector(8.f, 8.f, 8.f), FColor::Red);

		/**
		for (int32 ModifierIdx = 0; ModifierIdx < PlayerCamera->DefaultModifiers.Num(); ++ModifierIdx)
		{
			// Apply camera modification and output into DesiredCameraOffset/DesiredCameraRotation
			if (PlayerCamera->DefaultModifiers[ModifierIdx])
			{
				GEngine->AddOnScreenDebugMessage(12 + ModifierIdx, 1.0f, FColor::White, FString::Printf(TEXT("	CamModifier: %s"), *GetNameSafe(PlayerCamera->DefaultModifiers[ModifierIdx])));
			}
		}
		**/
	}
#endif
}

void UGameThirdPersonCamera::Init()
{
	if (!ThirdPersonCamDefault && ThirdPersonCamDefaultClass.GetDefaultObject())
	{
		ThirdPersonCamDefault = CreateCameraMode(ThirdPersonCamDefaultClass);
	}
}

float UGameThirdPersonCamera::GetDesiredFOV(APawn* ViewedPawn)
{
	if (CurrentCamMode)
	{
		return CurrentCamMode->GetDesiredFOV(ViewedPawn);
	}

	return 0.f;
}

void UGameThirdPersonCamera::UpdateCamera(APawn* P, class ASandboxCameraManager* CameraActor, float DeltaTime, FTViewTarget& OutVT)
{
	if (!P && OutVT.Target)
	{
		OutVT.Target->GetActorEyesViewPoint(OutVT.POV.Location, OutVT.POV.Rotation);
		PlayerCamera->ApplyCameraModifiers(DeltaTime, OutVT.POV);
	}
	else
	{
		UpdateCameraMode(P);
		if (CurrentCamMode)
		{
			PlayerUpdateCamera(P, CameraActor, DeltaTime, OutVT);
		}
#if WITH_EDITOR
		else
		{
			UE_LOG(SandboxCameraLog, Error, TEXT("UpdateCamera Error CurrentCamMode"))
		}
#endif
	}
	bResetCameraInterpolation = false;
}

void UGameThirdPersonCamera::BeginTurn(float StartAngle, float EndAngle, float TimeSec, float DelaySec /*= 0.f*/, bool bAlignTargetWhenFinished /*= false*/)
{
	bDoingACameraTurn = true;
	TurnTotalTime = TimeSec;
	TurnDelay = DelaySec;
	TurnCurTime = 0.f;
	TurnStartAngle = StartAngle;
	TurnEndAngle = EndAngle;
	bTurnAlignTargetWhenFinished = bAlignTargetWhenFinished;
}

void UGameThirdPersonCamera::EndTurn()
{
	bDoingACameraTurn = false;

	// maybe align
	if (bTurnAlignTargetWhenFinished)
	{
		if (PlayerCamera && PlayerCamera->PCOwner)
		{
			FRotator TmpRot = PlayerCamera->PCOwner->GetControlRotation();
			TmpRot.Yaw = LastPostCamTurnYaw;
			PlayerCamera->PCOwner->SetControlRotation(TmpRot);
		}
	}
}


void UGameThirdPersonCamera::AdjustTurn(float& AngleOffset)
{
	TurnStartAngle += AngleOffset;
	TurnEndAngle += AngleOffset;
}



UGameThirdPersonCameraMode* UGameThirdPersonCamera::FindBestCameraMode(APawn* P)
{
	if (P)
	{
		return ThirdPersonCamDefault;
	}
	return nullptr;
}

void UGameThirdPersonCamera::UpdateCameraMode(APawn* P)
{
	UGameThirdPersonCameraMode* NewCameraMode = FindBestCameraMode(P);
	if (NewCameraMode != CurrentCamMode)
	{
		if (CurrentCamMode)
		{
			CurrentCamMode->OnBecomeInActiveMode(P, NewCameraMode);
		}
		if (NewCameraMode)
		{
			NewCameraMode->OnBecomeActiveMode(P, CurrentCamMode);
		}
#if WITH_EDITOR
		bDebugChangedCameraMode = true;
#endif
		CurrentCamMode = NewCameraMode;
	}
}

void UGameThirdPersonCamera::ProcessViewRotation(float DeltaTime, AActor* ViewTarget, FRotator& out_ViewRotation, FRotator& out_DeltaRot)
{
	if (CurrentCamMode)
	{
		CurrentCamMode->ProcessViewRotation(DeltaTime, ViewTarget, out_ViewRotation, out_DeltaRot);
	}
}

void UGameThirdPersonCamera::OnBecomeActive(UGameCameraBase* OldCamera)
{
	if (!PlayerCamera->bInterpolateCamChanges)
	{
		Reset();
	}

	Super::OnBecomeActive(OldCamera);
}

void UGameThirdPersonCamera::ResetInterpolation()
{
	Super::ResetInterpolation();

	LastHeightAdjustment = 0.f;
	LastYawAdjustment = 0.f;
	LastPitchAdjustment = 0.f;
	LeftoverPitchAdjustment = 0.f;
}

void UGameThirdPersonCamera::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.SetDrawColor(FColor::Blue);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("UGameThirdPersonCamera %s"), *GetNameSafe(this)));
	if (CurrentCamMode)
	{
		CurrentCamMode->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	}
}




/**
* ASandboxCameraManager
**/
ASandboxCameraManager::ASandboxCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//	APlayerCameraManager settings
	DefaultFOV = 70.f;

	CameraStyle = CAMERA_THIRDPERSON;
	ThirdPersonCameraClass = UGameThirdPersonCamera::StaticClass();
	FixedCameraClass = UGameFixedCamera::StaticClass();
	ViewPitchMin = -66.0f;
	ViewPitchMax = 55.0f;

	bDebugCamera = false;
	bProcessViewRotation = false;
}

void ASandboxCameraManager::AddPawnToHiddenActorsArray(APawn* PawnToHide)
{
	if (PawnToHide && PCOwner)
	{
		PCOwner->HiddenActors.AddUnique(PawnToHide->GetOwner());
	}
}

void ASandboxCameraManager::RemovePawnFromHiddenActorsArray(APawn* PawnToShow)
{
	if (PawnToShow && PCOwner)
	{
		PCOwner->HiddenActors.Remove(PawnToShow);
	}
}

UGameCameraBase* ASandboxCameraManager::CreateCamera(TSubclassOf<UGameCameraBase> CameraClass)
{
	UGameCameraBase* NewCamera = NewObject<UGameCameraBase>(this, CameraClass);
	if (NewCamera)
	{
		NewCamera->PlayerCamera = this;
		NewCamera->Init();
	}
	return NewCamera;
}

void ASandboxCameraManager::CacheLastTargetBaseInfo(AActor* TargetBase)
{
	LastTargetBase = TargetBase;
	if (TargetBase)
	{
		LastTargetBaseTM = TargetBase->GetTransform().ToMatrixWithScale();
	}
}

void ASandboxCameraManager::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (!FixedCam && FixedCameraClass)
	{
		FixedCam = CreateCamera(FixedCameraClass);
	}

	if (!ThirdPersonCam && ThirdPersonCameraClass)
	{
		ThirdPersonCam = CreateCamera(ThirdPersonCameraClass);
	}
}

void ASandboxCameraManager::Reset()
{
	bUseForcedCamFOV = false;
}

UGameCameraBase* ASandboxCameraManager::FindBestCameraType(AActor* CameraTarget)
{
	UGameCameraBase* BestCamera = nullptr;
	if (CameraStyle == CAMERA_THIRDPERSON)
	{
		ACameraActor* CameraActor = Cast<ACameraActor>(CameraTarget);
		if (CameraActor)
		{
			BestCamera = FixedCam;
		}
		else
		{
			BestCamera = ThirdPersonCam;
		}
	}
	return BestCamera;
}

bool ASandboxCameraManager::ShouldConstrainAspectRatio()
{
	return false;
}

float ASandboxCameraManager::AdjustFOVForViewport(float inHorizFOV, APawn* CameraTargetPawn) const
{
	float OutFOV = inHorizFOV;

	if (CameraTargetPawn)
	{
		APlayerController* const PlayerOwner = Cast<APlayerController>(CameraTargetPawn->GetController());
		ULocalPlayer* const LP = (PlayerOwner != nullptr) ? Cast<ULocalPlayer>(PlayerOwner->Player) : nullptr;
		UGameViewportClient* const VPClient = (LP != nullptr) ? LP->ViewportClient : nullptr;
		if (VPClient && (VPClient->GetCurrentSplitscreenConfiguration() == ESplitScreenType::TwoPlayer_Vertical))
		{
			FVector2D FullViewportSize(0, 0);
			VPClient->GetViewportSize(FullViewportSize);

			float const BaseAspect = FullViewportSize.X / FullViewportSize.Y;
			bool bWideScreen = false;
			if ((BaseAspect > (16.f / 9.f - 0.01f)) && (BaseAspect < (16.f / 9.f + 0.01f)))
			{
				bWideScreen = false;
			}

			FVector2D PlayerViewportSize(FVector2D::ZeroVector);
			PlayerViewportSize.X = FullViewportSize.X * LP->Size.X;
			PlayerViewportSize.Y = FullViewportSize.Y * LP->Size.Y;

			float NewAspectRatio = PlayerViewportSize.X / PlayerViewportSize.Y;
			OutFOV = (NewAspectRatio / BaseAspect) * FMath::Tan(inHorizFOV * 0.5f * PI / 180.f);
			OutFOV = 2.f * FMath::Tan(OutFOV) * 180.f / PI;
		}
	}
	return OutFOV;
}

void ASandboxCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	if (PendingViewTarget.Target && BlendParams.bLockOutgoing && OutVT.Equal(ViewTarget))
	{
		return;
	}
	FMinimalViewInfo OrigPOV = OutVT.POV;
	if (!OutVT.Target)
	{
		UE_LOG(SandboxCameraLog, Error, TEXT("ASandboxCameraManager Error Target"))
		return;
	}
	UGameCameraBase* NewCamera = FindBestCameraType(OutVT.Target);
	if (CurrentCamera != NewCamera)
	{
		if (CurrentCamera)
		{
			CurrentCamera->OnBecomeInActive(NewCamera);
		}
		if (NewCamera)
		{
			NewCamera->OnBecomeActive(CurrentCamera);
		}
		CurrentCamera = NewCamera;
	}

	APawn* Pawn = Cast<APawn>(OutVT.Target);
	if (CurrentCamera)
	{
		if (bResetInterp && !bInterpolateCamChanges)
		{
			CurrentCamera->ResetInterpolation();
		}

		if (ACameraActor* CameraActor = Cast<ACameraActor>(OutVT.Target))
		{
			CameraActor->GetCameraComponent()->GetCameraView(DeltaTime, OutVT.POV);
			if (CurrentCamera == FixedCam && CameraActor->GetCameraComponent()->bConstrainAspectRatio)
			{
				OutVT.POV.bConstrainAspectRatio = true;
				OutVT.POV.AspectRatio = CameraActor->GetCameraComponent()->AspectRatio;
			}
		}
		CurrentCamera->UpdateCamera(Pawn, this, DeltaTime, OutVT);
		if (CameraStyle == CAMERA_FREE)
		{
			Super::UpdateViewTarget(OutVT, DeltaTime);
		}
	}
	else
	{
		Super::UpdateViewTarget(OutVT, DeltaTime);
	}

	// FOV
	if (bUseForcedCamFOV)
	{
		OutVT.POV.FOV = ForcedCamFOV;
	}
	OutVT.POV.FOV = AdjustFOVForViewport(OutVT.POV.FOV, Pawn);
	SetActorLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation, false);
	UpdateCameraLensEffects(OutVT);
	CacheLastTargetBaseInfo(OutVT.Target);
	bResetInterp = false;
}

void ASandboxCameraManager::ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot)
{
	Super::ProcessViewRotation(DeltaTime, OutViewRotation, OutDeltaRot);
	if (CurrentCamera && bProcessViewRotation)
	{
		CurrentCamera->ProcessViewRotation(DeltaTime, ViewTarget.Target, OutViewRotation, OutDeltaRot);
	}
}

void ASandboxCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	if (CurrentCamera)
	{
		FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
		DisplayDebugManager.SetDrawColor(FColor::Magenta);
		DisplayDebugManager.DrawString(FString::Printf(TEXT("ASandboxCameraManager %s"), *GetNameSafe(this)));
		CurrentCamera->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	}
}