
#include "SandboxGameCamera.h"
#include "Engine/Canvas.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/MovementComponent.h"
#include "Camera/CameraAnimInst.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SandboxPawn.h"
#include "SandboxController.h"


/**
 * USandboxGameplayCameraMode
 */
USandboxGameplayCameraMode::USandboxGameplayCameraMode()
{
	SpectatorCameraRotInterpSpeed = 14.f;
	FOVAngle = 80;
}

FVector USandboxGameplayCameraMode::InterpolateCameraOriginLoc(APawn* TargetPawn, FRotator const& CameraTargetRot, FVector const& LastLoc, FVector const& IdealLoc, float DeltaTime)
{
	return Super::InterpolateCameraOriginLoc(TargetPawn, CameraTargetRot, LastLoc, IdealLoc, DeltaTime);

}

FRotator USandboxGameplayCameraMode::InterpolateCameraOriginRot(APawn* TargetPawn, FRotator const& LastRot, FRotator const& IdealRot, float DeltaTime)
{
	if (GameplayCam && GameplayCam->PlayerCamera)
	{
		ASandboxController* const GPC = Cast<ASandboxController>(GameplayCam->PlayerCamera->PCOwner);
		if (GPC && GPC->IsSpectating())
		{
			return FMath::RInterpTo(LastRot, IdealRot, DeltaTime, SpectatorCameraRotInterpSpeed);
		}
	}
	return Super::InterpolateCameraOriginRot(TargetPawn, LastRot, IdealRot, DeltaTime);
}

float USandboxGameplayCameraMode::GetViewPitch(APawn * TargetPawn, FRotator const & ViewRotation) const
{
	return Super::GetViewPitch(TargetPawn, ViewRotation);
}

void USandboxGameplayCameraMode::GetCameraOrigin(APawn * TargetPawn, FVector & OriginLoc, FRotator & OriginRot)
{
	Super::GetCameraOrigin(TargetPawn, OriginLoc, OriginRot);
	ASandboxPawn* const GP = Cast<ASandboxPawn>(TargetPawn);
	if (GP)
	{
		FVector const SuperOriginLoc = OriginLoc;
		OriginLoc = TargetPawn->GetActorLocation();
		OriginLoc.Z += TargetPawn->BaseEyeHeight;
		if (GP->bIsCrouched)
		{
			OriginLoc.Z += GP->GetCharacterMovement()->CrouchedHalfHeight * 0.5f;
		}

		OriginLoc = TargetPawn->GetActorLocation();
		OriginLoc.Z += TargetPawn->BaseEyeHeight;
		if (GP->bIsCrouched)
		{
			OriginLoc.Z += GP->GetCharacterMovement()->CrouchedHalfHeight * 0.5f;
		}

		if (SuperOriginLoc != OriginLoc)
		{
			OriginLoc += TransformLocalToWorld(GetTargetRelativeOriginOffset(TargetPawn), TargetPawn->GetViewRotation());
		}
		OriginLoc += GP->MTO_MeshLocSmootherOffset;
	}

	if (bTemporaryPivotInterp)
	{
		float Alpha = FMath::FInterpTo(LastTemporaryPivotInterpAlpha, 1.f, GetWorld()->GetDeltaSeconds(), TemporaryPivotInterpSpeed);
		LastTemporaryPivotInterpAlpha = Alpha;

		OriginRot = RotatorLerp(TemporaryPivotInterpStartingRotation, OriginRot, Alpha);

		if (Alpha >= 1.f)
		{
			bTemporaryPivotInterp = false;
		}
	}
}

void USandboxGameplayCameraMode::GetBaseViewOffsets(APawn* ViewedPawn, TEnumAsByte<ECameraViewportTypes> ViewportConfig, float DeltaTime, FVector& out_Low, FVector& out_Mid, FVector& out_High)
{
	FVector StrafeOffset(FVector::ZeroVector);
	FVector RunOffset(FVector::ZeroVector);
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
	if (ViewedPawn->Controller)
	{
		FVector UnusedVec(FVector::ZeroVector);
		ViewedPawn->Controller->GetPlayerViewPoint(UnusedVec, CamRot);
	}
	else
	{
		CamRot = ViewedPawn->GetActorRotation();
	}
	FVector TotalOffset = StrafeOffset + RunOffset;
	TotalOffset = ::TransformWorldToLocal(TotalOffset, ViewedPawn->GetActorRotation());
	TotalOffset = ::TransformLocalToWorld(TotalOffset, CamRot);
	ASandboxPawn* GP = Cast<ASandboxPawn>(ViewedPawn);
	const bool bIsMirrored = (GP != nullptr && GP->bIsMirrored);
	const float MirrorScale = bIsMirrored ? -1.f : 1.f;
	TotalOffset.Y *= MirrorScale;
	out_Low = ViewOffset.OffsetLow + TotalOffset;
	out_Mid = ViewOffset.OffsetMid + TotalOffset;
	out_High = ViewOffset.OffsetHigh + TotalOffset;
}

FVector USandboxGameplayCameraMode::AdjustViewOffset(APawn* P, FVector& Offset)
{
	ASandboxPawn* const GP = Cast<ASandboxPawn>(P);
	if (GP)
	{
		Offset += GP->ExtraThirdPersonCameraOffset;

		ASandboxPawn const* const CDO = Cast<ASandboxPawn>(GP->GetClass()->GetDefaultObject());
		if (GP->bIsMirrored)
		{
			Offset.Y = -Offset.Y;
		}
	}
	return Offset;
}

float USandboxGameplayCameraMode::GetDesiredFOV(APawn* ViewedPawn)
{
	if (ASandboxPawn* GP = Cast<ASandboxPawn>(ViewedPawn))
	{
		ASandboxWeapon* GW = Cast<ASandboxWeapon>(GP->GetCurrentWeapon());
		return GW->GetAdjustedFOV(FOVAngle) + FOVAngle_ViewportAdjustments[CurrentViewportType];
	}
	else
	{
		return FOVAngle + FOVAngle_ViewportAdjustments[CurrentViewportType];
	}
}

void USandboxGameplayCameraMode::WeaponChanged(ASandboxController* C, ASandboxWeapon* OldWeapon, ASandboxWeapon* NewWeapon)
{
	//nothing
}

FVector USandboxGameplayCameraMode::GetCameraWorstCaseLoc(APawn* TargetPawn, FTViewTarget& CurrentViewTarget)
{
	FVector WorstLocation(FVector::ZeroVector);
	FVector PerPawnWorstLocOffset(FVector::ZeroVector);
	FVector LocalWorstLocOffset(FVector::ZeroVector);

	if (ASandboxPawn* GP = Cast<ASandboxPawn>(TargetPawn))
	{
		LocalWorstLocOffset = WorstLocOffset + PerPawnWorstLocOffset;
		if (GP->bIsMirrored)
		{
			LocalWorstLocOffset.Y = -(LocalWorstLocOffset.Y);
		}
		WorstLocation = GP->GetActorLocation() + GP->GetActorRotation().RotateVector(LocalWorstLocOffset);
	}
	else
	{
		WorstLocation = TargetPawn->GetActorLocation();
	}

	return WorstLocation;
}

void USandboxGameplayCameraMode::StartTransitionPivotInterp()
{
	LastTemporaryPivotInterpAlpha = 0.f;
	TemporaryPivotInterpStartingRotation = GameplayCam->LastActualCameraOriginRot;
	bTemporaryPivotInterp = true;
}

void USandboxGameplayCameraMode::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("SandboxGameplayCameraMode %s"), *GetNameSafe(this)));
}





/**
 * USandboxGameplayCamDefault
 */
USandboxGameplayCamDefault::USandboxGameplayCamDefault()
{
	TemporaryOriginRotInterpSpeed = 12.f;

	WorstLocOffset = FVector(-8.f, 1.f, 95.f);
	WorstLocAimingZOffset = -10.f;
	bValidateWorstLoc = false;

	bUsePerAxisOriginLocInterp = true;
	PerAxisOriginLocInterpSpeed = FVector(5.f, 20.f, 20.f);

	ViewOffsetAdjustment_Evade = FVector(0, 0, -28.f);

	ViewOffset.OffsetHigh = FVector(-128.f, 56.f, 40.f);
	ViewOffset.OffsetLow = FVector(-160.f, 48.f, 56.f);
	ViewOffset.OffsetMid = FVector(-160.f, 48.f, 16.f);


	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_HorizSplit].OffsetHigh = FVector(0, 0, -12);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_HorizSplit].OffsetLow = FVector(0, 0, -12);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_HorizSplit].OffsetMid = FVector(0, 0, -12);

	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_VertSplit].OffsetHigh = FVector(70, -30, 20);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_VertSplit].OffsetLow = FVector(70, -30, 20);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_VertSplit].OffsetMid = FVector(70, -30, 20);

	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_Full].OffsetHigh = FVector(0, 0, 17);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_Full].OffsetLow = FVector(0, 0, 17);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_Full].OffsetMid = FVector(0, 0, 17);

	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_HorizSplit].OffsetHigh = FVector(0, 0, -15);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_HorizSplit].OffsetLow = FVector(0, 0, -15);
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_HorizSplit].OffsetMid = FVector(0, 0, -15);

	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_VertSplit].OffsetHigh = FVector::ZeroVector;
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_VertSplit].OffsetLow = FVector::ZeroVector;
	ViewOffset_ViewportAdjustments[ECameraViewportTypes::CVT_4to3_VertSplit].OffsetMid = FVector::ZeroVector;

	FOVAngle_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_VertSplit] = 40.f;

	BlendTime = 0.67f;
	FOVAngle = 80.f;
}

FVector USandboxGameplayCamDefault::AdjustViewOffset(APawn* P, FVector& Offset)
{
	ASandboxPawn* GP = Cast<ASandboxPawn>(P);
	if (GP && GP->IsEvading())
	{
		Offset += ViewOffsetAdjustment_Evade;
	}

	return Super::AdjustViewOffset(P, Offset);
}

FVector USandboxGameplayCamDefault::GetCameraWorstCaseLoc(APawn* TargetPawn, FTViewTarget& CurrentViewTarget)
{
	FVector WorstLocation = Super::GetCameraWorstCaseLoc(TargetPawn, CurrentViewTarget);
	ASandboxPawn* GP = Cast<ASandboxPawn>(TargetPawn);
	if (GP && GP->IsInAimingPose())
	{
		WorstLocation.Z += WorstLocAimingZOffset;
	}
	return WorstLocation;
}

void USandboxGameplayCamDefault::OnBecomeActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* PrevMode)
{
	Super::OnBecomeActiveMode(TargetPawn, PrevMode);
}




/**
 * USandboxGameplayRoadieRun
 */
USandboxGameplayRoadieRun::USandboxGameplayRoadieRun()
{
	PerAxisOriginLocInterpSpeed = FVector(8.f, 20.f, 20.f);
	ViewOffset.OffsetHigh = FVector(0.f, 56.f, 50.f);
	ViewOffset.OffsetMid = FVector(-88.f, 48.f, -40.f);
	ViewOffset.OffsetLow = FVector(-88.f, 56.f, -40.f);
	FOVAngle_ViewportAdjustments[CVT_16to9_VertSplit] = 20.f;
	RunFwdAdjustment = FVector::ZeroVector;
	RunBackAdjustment = FVector::ZeroVector;
	WorstLocOffset = FVector(-20, 10, 60);
	bDoPredictiveAvoidance = false;
	bValidateWorstLoc = false;
	BoomerBlendTime = 0.65f;
	FOVAngle = 105.f;
}

void USandboxGameplayRoadieRun::OnBecomeActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* PrevMode)
{
	Super::OnBecomeActiveMode(TargetPawn, PrevMode);
}






/**
 * USandboxGameplayTargeting
 */
USandboxGameplayTargeting::USandboxGameplayTargeting()
{
	bUsePerAxisOriginLocInterp = true;
	PerAxisOriginLocInterpSpeed = FVector(0, 0, 8);
	ViewOffset.OffsetHigh = FVector(-40, 40, 24);
	ViewOffset.OffsetLow = FVector(-125, 45, 20);
	ViewOffset.OffsetMid = FVector(-70, 40, 6);
	FOVAngle_ViewportAdjustments[ECameraViewportTypes::CVT_16to9_VertSplit] = 30.f;
	ViewOffsetAdjustment_Crouch = FVector(0, 0, -15);
	ViewOffsetAdjustment_Kidnapper = FVector(0, 0, 10);
	WorstLocOffset = FVector(0, 25, 70);
	BlendTime = 0.067f;
	FOVAngle = 50.f;
}

float USandboxGameplayTargeting::GetDesiredFOV(APawn* ViewedPawn)
{
	if (ASandboxPawn* GP = Cast<ASandboxPawn>(ViewedPawn))
	{
		if (ASandboxWeapon* GW = Cast<ASandboxWeapon>(GP->GetCurrentWeapon()))
		{
			return GW->GetTargetingFOV(FOVAngle) + FOVAngle_ViewportAdjustments[CurrentViewportType];
		}
	}
	return Super::GetDesiredFOV(ViewedPawn);
}

void USandboxGameplayTargeting::OnBecomeActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* PrevMode)
{
	Super::OnBecomeActiveMode(TargetPawn, PrevMode);
}

void USandboxGameplayTargeting::OnBecomeInActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* NewMode)
{
	Super::OnBecomeInActiveMode(TargetPawn, NewMode);
}

void USandboxGameplayTargeting::WeaponChanged(ASandboxController* C, ASandboxWeapon* OldWeapon, ASandboxWeapon* NewWeapon)
{
	if (C && C->IsLocalPlayerController())
	{
		if (OldWeapon && OldWeapon->GetWeaponMesh())
		{
			OldWeapon->GetWeaponMesh()->SetViewOwnerDepthPriorityGroup(false, ESceneDepthPriorityGroup::SDPG_World);
		}

		if (NewWeapon && NewWeapon->GetWeaponMesh())
		{
			NewWeapon->GetWeaponMesh()->SetViewOwnerDepthPriorityGroup(true, ESceneDepthPriorityGroup::SDPG_Foreground);
		}
	}
}

FVector USandboxGameplayTargeting::AdjustViewOffset(APawn* P, FVector& Offset)
{
	ASandboxPawn* GP = Cast<ASandboxPawn>(P);
	if (GP->GetCharacterMovement()->IsCrouching() && (GP->GetVelocity().Size() < 1))
	{
		Offset += ViewOffsetAdjustment_Crouch;
	}

	return Super::AdjustViewOffset(P, Offset);
}






/**
 * UGameplayCameraMode
 */
USandboxGameplayCamera::USandboxGameplayCamera()
{
	ThirdPersonCamDefaultClass = USandboxGameplayCamDefault::StaticClass();
	HideViewTargetExtent = FVector(22, 22, 22);
	HideViewTargetExtent_FlickerBuffer = FVector(2.0f, 2.0f, 2.0f);
}

bool USandboxGameplayCamera::ShouldDoPerPolyPenetrationTests(APawn* TargetPawn) const
{
	return Super::ShouldDoPerPolyPenetrationTests(TargetPawn);
}

bool USandboxGameplayCamera::ShouldDoPredictavePenetrationAvoidance(APawn* TargetPawn) const
{
	return Super::ShouldDoPredictavePenetrationAvoidance(TargetPawn);
}

FVector USandboxGameplayCamera::GetHideViewTargetExtent() const
{
	return HideViewTargetExtent;
}

void USandboxGameplayCamera::HandlePawnPenetration(FTViewTarget& OutVT)
{
	if (PlayerCamera && PlayerCamera->PCOwner->IsLocalPlayerController())
	{
		for (TObjectIterator<AActor> Itr; Itr; ++Itr)
		{
			if (ASandboxPawn* TempGP = Cast<ASandboxPawn>(*Itr))
			{
				if (!TempGP->IsAliveAndWell())
				{
					PlayerCamera->RemovePawnFromHiddenActorsArray(TempGP);
				}
				else
				{
					bool bInsideTargetCylinder = false;
					float CylRadSq = 0;
					float HalfCylHeight = 0;
					if (TempGP)
					{
						TempGP->GetSimpleCollisionCylinder(CylRadSq, HalfCylHeight);
					}
					CylRadSq *= CylRadSq;
					FVector TempPEffectiveLoc = TempGP->GetActorLocation();
					switch (TempGP->CoverAction)
					{
					case ECoverAction::CA_LeanRight:
					case ECoverAction::CA_LeanLeft:
						{
							//follow xy pelvis
							FVector PelvisWorldPos = TempGP->GetMesh()->GetBoneLocation(TempGP->NeckBoneName);
							TempPEffectiveLoc.X = PelvisWorldPos.X;
							TempPEffectiveLoc.Y = PelvisWorldPos.Y;
						}
					}

					if ((OutVT.POV.Location - TempPEffectiveLoc).SizeSquared2D() < CylRadSq)
					{
						float const CylMaxZ = TempPEffectiveLoc.Z + HalfCylHeight;
						float const CylMinZ = TempPEffectiveLoc.Z - HalfCylHeight;

						if ((OutVT.POV.Location.Z < CylMaxZ) && (OutVT.POV.Location.Z > CylMinZ))
						{
							bInsideTargetCylinder = true;
						}
					}
					if (bInsideTargetCylinder)
					{
						PlayerCamera->AddPawnToHiddenActorsArray(TempGP);
					}
					else
					{
						PlayerCamera->RemovePawnFromHiddenActorsArray(TempGP);
					}
				}
			}
		}
	}
}

FVector USandboxGameplayCamera::GetPostInterpCameraOriginLocationOffset(APawn* TargetPawn) const
{
	if (ASandboxPawn* const GP = Cast<ASandboxPawn>(TargetPawn))
	{
		return (FRotationMatrix(GP->GetActorRotation()).TransformVector(GP->FloorConformTranslation) + GP->MTO_SpecialMoveInterp);
	}
	return FVector::ZeroVector;
}

FMatrix USandboxGameplayCamera::GetWorstCaseLocTransform(APawn* P) const
{
	return FRotationTranslationMatrix(P->GetActorRotation(), P->GetActorLocation());
}

void USandboxGameplayCamera::UpdateForMovingBase(UPrimitiveComponent* BaseActor)
{
	Super::UpdateForMovingBase(BaseActor);
}

bool USandboxGameplayCamera::ShouldIgnorePenetrationHit(FHitResult Hit, APawn* TargetPawn) const
{
	return Super::ShouldIgnorePenetrationHit(Hit, TargetPawn);
}

FRotator USandboxGameplayCamera::GetOriginRotThatPointsCameraAtPoint(FVector PivotPos, FRotator PivotRot, FVector CameraPos, FRotator CameraRot, FVector LookatWorld)
{
	if (((PivotPos - CameraPos).Size() * 1.3f) > (PivotPos - LookatWorld).Size())
	{
		return PivotRot;
	}
	FMatrix CameraToWorld = FRotationTranslationMatrix(CameraRot, CameraPos);
	FMatrix PivotToWorld = FRotationTranslationMatrix(PivotRot, PivotPos);
	FMatrix const CameraToPivot = CameraToWorld * PivotToWorld.InverseFast();
	for (uint32 IterationIdx = 0; IterationIdx < 3; IterationIdx++)
	{
		CameraPos = CameraToWorld.GetOrigin();
		FRotator CamToLookatRot = (LookatWorld - CameraPos).Rotation();
		FRotationTranslationMatrix RotatedCamToWorld(CamToLookatRot, CameraPos);
		FMatrix RotAdjustment = RotatedCamToWorld * CameraToWorld.InverseFast();
		FRotator R = RotAdjustment.Rotator();
		R.Roll = 0;
		RotAdjustment = FRotationMatrix(R);
		PivotToWorld = RotAdjustment * PivotToWorld;
		CameraToWorld = CameraToPivot * PivotToWorld;
	}

	return PivotToWorld.Rotator();
}



void USandboxGameplayCamera::Init()
{
	bool bCameraBindingsAdded = false;
	if (!bCameraBindingsAdded)
	{
		SandCamTargeting = CreateCameraMode(USandboxGameplayTargeting::StaticClass());
		SandCamRoadie= CreateCameraMode(USandboxGameplayRoadieRun::StaticClass());
	}
	Super::Init();
}

UGameThirdPersonCameraMode* USandboxGameplayCamera::CreateCameraMode(TSubclassOf<UGameThirdPersonCameraMode> ModeClass)
{
	UGameThirdPersonCameraMode* NewMode = Super::CreateCameraMode(ModeClass);
	USandboxGameplayCameraMode* SandCameraMode = Cast<USandboxGameplayCameraMode>(NewMode);
	if (SandCameraMode)
	{
		SandCameraMode->GameplayCam = this;
	}
	return NewMode;
}

UGameThirdPersonCameraMode* USandboxGameplayCamera::FindBestCameraMode(APawn* P)
{
	if (!P)
		return nullptr;

	UGameThirdPersonCameraMode* NewCamMode = nullptr;
	if (ASandboxPawn* GP = Cast<ASandboxPawn>(P))
	{
		LastPawn = GP;
		if (GP->IsTargeting())
		{
			NewCamMode = SandCamTargeting;
		}
		else if (GP->bIsRoadieRunning)
		{
			NewCamMode = SandCamRoadie;
		}
		else
		{
			NewCamMode = ThirdPersonCamDefault;
		}
	}
	return NewCamMode;
}

void USandboxGameplayCamera::OnBecomeInActive(UGameCameraBase* NewCamera)
{
	if (CurrentCamMode)
	{
		CurrentCamMode->OnBecomeInActiveMode(nullptr, nullptr);
	}

	SnapToLookatPointTimeRemaining = 0.f;
}

void USandboxGameplayCamera::ForceNextFrameLookatPoint(FVector LookatPointWorld, float Duration)
{
	SnapToLookatPointWorld = LookatPointWorld;
	SnapToLookatPointTimeRemaining = Duration;
	LastSnapToLookatDeltaRot = FRotator::ZeroRotator;
	if (ASandboxPawn* GP = Cast<ASandboxPawn>(PlayerCamera->PCOwner->GetPawn()))
	{
		SnapToLookatCachedCoverAction = GP->CoverAction;
		SnapToLookatCachedCoverDir = GP->CoverDirection;
	}
}

void USandboxGameplayCamera::UpdateCamera(APawn* P, class ASandboxCameraManager* CameraActor, float DeltaTime, FTViewTarget& OutVT)
{
	Super::UpdateCamera(P, CameraActor, DeltaTime, OutVT);

	if (SnapToLookatPointTimeRemaining > 0.f)
	{
		FRotator DesiredPivotRot = GetOriginRotThatPointsCameraAtPoint(LastIdealCameraOriginLoc, LastIdealCameraOriginRot, OutVT.POV.Location, OutVT.POV.Rotation, SnapToLookatPointWorld);
		FRotator DeltaRot = DesiredPivotRot - LastIdealCameraOriginRot;
		DeltaRot.Normalize();
		DeltaRot.Roll = 0;
		if (DeltaRot.IsNearlyZero())
		{
			// stop
			SnapToLookatPointTimeRemaining = 0.f;
		}
		else
		{
			if (ASandboxController* GPC = Cast<ASandboxController>(PlayerCamera->PCOwner))
			{
				GPC->ExtraRot += DeltaRot;
				SnapToLookatPointTimeRemaining -= DeltaTime;
			}
		}
		LastSnapToLookatDeltaRot = DeltaRot;
	}
}

void USandboxGameplayCamera::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.SetDrawColor(FColor::Red);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("UGameThirdPersonCameraMode %s"), *GetNameSafe(this)));
	
	if (CurrentCamMode)
	{
		CurrentCamMode->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	}
}






/**
 * USandboxFixedCamera
 */
USandboxFixedCamera::USandboxFixedCamera()
{
	//
}







/**
 * USandboxCameraBone
 */
USandboxCameraBone::USandboxCameraBone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CameraBoneName = TEXT("Camera");
	CameraBoneRootName = TEXT("Camera_Root");
	CameraAnimOption = ECameraAnimOption::CAO_TranslateRotateDelta;
	AlphaInterpSpeed = 7.0f;
	AlphaInTime = 0.f;
	AlphaOutTime = 0.f;
	MaxTranslation = FVector(2000.f, 2000.f, 2000.f);
	bShakyCamDampening = true;
	ShakyCamDampeningFactor = 0.125f;
	AlphaWhenIdle = 0.3f;
	InCombatAlphaBonus = 0.05f;
}

FRotator USandboxCameraBone::CalcDeltaRot(FRotator CameraRootRot, FRotator CameraBoneRot, FRotator PawnRot)
{
	FVector X, Y, Z;
	FRotationMatrix RootMat(CameraRootRot);
	RootMat.GetUnitAxes(X, Y, Z);
	X = -X;
	RootMat.SetAxes(&Y, &X, &Z);
	FRotator FixedCameraRootRot = RootMat.Rotator();

	FRotationMatrix BoneMat(CameraBoneRot);
	BoneMat.GetUnitAxes(X, Y, Z);
	X = -X;
	BoneMat.SetAxes(&Y, &X, &Z);
	FRotator FixedCameraBoneRot = BoneMat.Rotator();

	FRotator Delta = (FixedCameraBoneRot - FixedCameraRootRot).GetNormalized();
	return Delta;
}

FRotator USandboxCameraBone::FixCamBone(FRotator A) const
{
	FVector X, Y, Z;
	FRotationMatrix R(A);
	X = R.GetUnitAxis(EAxis::X);
	Y = R.GetUnitAxis(EAxis::Y);
	Z = R.GetUnitAxis(EAxis::Z);

	X = -X;

	FMatrix M = FMatrix::Identity;
	M.SetAxis(0, Y);
	M.SetAxis(1, X);
	M.SetAxis(2, Z);

	return M.Rotator();
}

float USandboxCameraBone::GetTargetAlpha()
{
	float RetAlpha = 0.f;
	const float SuperTargetAlpha = Super::GetTargetAlpha();
	if (SuperTargetAlpha != 0.f)
	{
		float AlphaBonus = 0.f;
		float Scale = 0.f;

		bool bInCombat = ((GetWorld()->GetTimeSeconds() - Pawn->LastShotAtTime) < 5.0f);
		if (bInCombat && !bDisableAmbientCameraMotion)
		{
			AlphaBonus += InCombatAlphaBonus;
		}

		if (Pawn->bIsRoadieRunning)
		{
			RetAlpha = Pawn->RoadieRunShakyCamDampeningFactor;
			Scale = 1.f;

			if (GEngine->IsSplitScreen(GetWorld()))
			{
				ASandboxGameCamera* PlayerCamera = Cast<ASandboxGameCamera>(CameraOwner);
				if (PlayerCamera)
				{
					Scale *= PlayerCamera->SplitScreenShakeScale;
				}
			}
		}
		else if (Pawn->GetCharacterMovement()->MaxWalkSpeed != 0.f)
		{
			Scale = bDisableAmbientCameraMotion ? 0.f : ShakyCamDampeningFactor;
			float CurrentSpeed = Pawn->GetVelocity().Size();

			if (Pawn->CoverType != ECoverType::CT_None)
			{
				RetAlpha = CurrentSpeed / (Pawn->GetCharacterMovement()->MaxWalkSpeed * Pawn->CoverMovementPct[Pawn->CoverType]) * (1.f - AlphaWhenIdle) + AlphaWhenIdle;
			}
			else
			{
				RetAlpha = CurrentSpeed / Pawn->GetCharacterMovement()->MaxWalkSpeed * (1.f - AlphaWhenIdle) + AlphaWhenIdle;
			}

			if (Pawn->IsTargeting())
			{
				Scale *= 0.25f;
				AlphaBonus = 0.f;
			}
			else if (Pawn->IsInAimingPose())
			{
				Scale *= 0.5f;
			}
			else if (!bInCombat && Pawn->GetVelocity().IsZero())
			{
				Scale = 0.f;
			}
		}
		RetAlpha = RetAlpha * Scale + AlphaBonus;
	}
	return RetAlpha;
}

bool USandboxCameraBone::Initialize()
{
	if (CameraOwner
		&& CameraOwner->PCOwner
		&& CameraOwner->PCOwner->GetPawn())
	{
		Pawn = CastChecked<ASandboxPawn>(CameraOwner->PCOwner->GetPawn());
		if (Pawn)
		{
			CameraBoneMesh = Pawn->GetMesh();
			return true;
		}
	}
	return false;
}

bool USandboxCameraBone::ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV)
{
	FRotator	DeltaRot(FRotator::ZeroRotator);
	FRotator	CameraBoneWorldRot(FRotator::ZeroRotator);
	FRotator	CameraBoneRootWorldRot(FRotator::ZeroRotator);
	FRotator	FixedCameraRootRot(FRotator::ZeroRotator);
	FVector		Delta(FVector::ZeroVector);
	FVector		NewBoneLocation(FVector::ZeroVector);
	FVector		CameraBoneRootWorldPos(FVector::ZeroVector);
	FVector		CameraBoneWorldPos(FVector::ZeroVector);

	LastDeltaPos = Delta;
	LastDeltaRot = DeltaRot;

	bool const bInitOK = Initialize();
	if (!bInitOK)
	{
		return false;
	}

	if ((!CameraBoneMesh)
		|| (CameraBoneMesh->GetBoneIndex(CameraBoneName) == INDEX_NONE)
		|| (CameraBoneMesh->GetBoneIndex(CameraBoneRootName) == INDEX_NONE))
	{
		return false;
	}

	UpdateAlpha(DeltaTime);
	Pawn->CameraBoneMotionScale = Alpha;

	if (Alpha > 0.f)
	{
		if (CameraAnimOption == ECameraAnimOption::CAO_TranslateDelta
			|| CameraAnimOption == ECameraAnimOption::CAO_TranslateRotateDelta)
		{
			CameraBoneWorldPos = CameraBoneMesh->GetBoneLocation(CameraBoneName);
			CameraBoneRootWorldPos = CameraBoneMesh->GetBoneLocation(CameraBoneRootName);
			CameraBoneRootWorldRot = FQuatRotationTranslationMatrix(CameraBoneMesh->GetBoneQuaternion(CameraBoneRootName), FVector(FVector::ZeroVector)).Rotator();
			FixedCameraRootRot = FixCamBone(CameraBoneRootWorldRot);
			Delta = (CameraBoneWorldPos - CameraBoneRootWorldPos) * Alpha;
			Delta = ClampVector(Delta, -MaxTranslation, MaxTranslation);
			Delta = FRotationMatrix(FixedCameraRootRot).GetTransposed().TransformVector(Delta);
			Delta = FRotationMatrix(Pawn->GetActorRotation()).TransformVector(Delta);

			if (CameraAnimOption == ECameraAnimOption::CAO_TranslateRotateDelta)
			{
				CameraBoneWorldRot = FQuatRotationTranslationMatrix(CameraBoneMesh->GetBoneQuaternion(CameraBoneName), FVector::ZeroVector).Rotator();
				DeltaRot = CalcDeltaRot(CameraBoneRootWorldRot, CameraBoneWorldRot, Pawn->GetActorRotation());
				DeltaRot *= Alpha;
			}
		}
		else if (CameraAnimOption == ECameraAnimOption::CAO_Absolute)
		{
			NewBoneLocation = CameraBoneMesh->GetBoneLocation(CameraBoneName);
			Delta = (NewBoneLocation - InOutPOV.Location) * Alpha;
		}
	}

	InOutPOV.Location += Delta;
	InOutPOV.Rotation += DeltaRot;
	LastDeltaPos = Delta;
	LastDeltaRot = DeltaRot;
	return false;
}

void USandboxCameraBone::UpdateAlpha(float DeltaTime)
{
	float PvsAlpha = Alpha;
	Super::UpdateAlpha(DeltaTime);
	Alpha = FMath::FInterpTo(PvsAlpha, Alpha, DeltaTime, AlphaInterpSpeed);
}





/**
 * ASandboxGameCamera
 */
ASandboxGameCamera::ASandboxGameCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ThirdPersonCameraClass = nullptr;
	FixedCameraClass = USandboxFixedCamera::StaticClass();
}

void ASandboxGameCamera::AddPawnToHiddenActorsArray(APawn* PawnToHide)
{
	Super::AddPawnToHiddenActorsArray(PawnToHide);
	if (ASandboxPawn* SP = Cast<ASandboxPawn>(PawnToHide))
	{
		SP->bIsHiddenByCamera = true;
	}
}

void ASandboxGameCamera::RemovePawnFromHiddenActorsArray(APawn* PawnToShow)
{
	Super::RemovePawnFromHiddenActorsArray(PawnToShow);
	ASandboxPawn* SP = Cast<ASandboxPawn>(PawnToShow);
	if (SP != nullptr)
	{
		SP->bIsHiddenByCamera = false;
	}
}

void ASandboxGameCamera::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (!GameplayCam)
	{
		UGameCameraBase* baseCamera = CreateCamera(USandboxGameplayCamera::StaticClass());
		GameplayCam = Cast<USandboxGameplayCamera>(baseCamera);
		if (!baseCamera || !GameplayCam)
		{
			UE_LOG(SandboxCameraLog, Error, TEXT("ASandboxGameCamera Error creating camera"))
		}
	}
	if (!SandboxCameraBone)
	{
		SandboxCameraBone = AddNewCameraModifier(USandboxCameraBone::StaticClass());
	}
}

UGameCameraBase* ASandboxGameCamera::FindBestCameraType(AActor* CameraTarget)
{
	UGameCameraBase* BestCam = nullptr;
	ASandboxPawn* TargetPawn = Cast<ASandboxPawn>(CameraTarget);
	if ((CameraStyle == CAMERA_THIRDPERSON) && TargetPawn)
	{
		if (ACameraActor* CameraActor = Cast<ACameraActor>(CameraTarget))
		{
			BestCam = FixedCam;
		}
		else
		{
			BestCam = GameplayCam;
		}
	}
	return BestCam;
}

void ASandboxGameCamera::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	FName CachedCameraStyle = CameraStyle;
	CameraStyle = CAMERA_THIRDPERSON;
	FMinimalViewInfo OrigPOV = OutVT.POV;
	// our custom camera implementation
	Super::UpdateViewTarget(OutVT, DeltaTime);
	CameraStyle = CachedCameraStyle;
	if (CameraStyle != CAMERA_THIRDPERSON)
	{
		OutVT.POV = OrigPOV;
		APlayerCameraManager::UpdateViewTarget(OutVT, DeltaTime);
	}
}

void ASandboxGameCamera::ApplyTransientCameraAnimScale(float Scale)
{
	for (int32 CameraAnimIdx = 0; CameraAnimIdx < ActiveAnims.Num(); CameraAnimIdx++)
	{
		ActiveAnims[CameraAnimIdx]->ApplyTransientScaling(Scale);
	}
}



