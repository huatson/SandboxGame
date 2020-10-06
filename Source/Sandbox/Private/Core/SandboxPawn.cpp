// Fill out your copyright notice in the Description page of Project Settings.


#include "SandboxPawn.h"
#include "GameFramework/PlayerInput.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Sandbox.h"
#include "SandboxPawnMovement.h"
#include "SandboxGameModeBase.h"
#include "SandboxAnimation.h"
#include "SandboxController.h"
#include "Net/UnrealNetwork.h"


#define MIN_MESHTRANSLATIONOFFSET	12.0f
#define MAX_MESHTRANSLATIONOFFSET	48.0f
#define MAX_MESHROTATIONOFFSET		10.0f
#define MAXSTEPHEIGHTFUDGE			2.0f
#define MINFLOORDIST				1.9f
#define MAXFLOORDIST				2.4f

ASandboxPawn::ASandboxPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USandboxPawnMovement>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	SetReplicateMovement(true);
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.0f, 0.f));
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCapsuleRadius(30.f);
	GetCapsuleComponent()->SetCapsuleHalfHeight(90.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);
	SandboxMovementComponent = Cast<USandboxPawnMovement>(GetMovementComponent());
	if (SandboxMovementComponent)
	{
		SandboxMovementComponent->UpdatedComponent = GetCapsuleComponent();
		CrouchedEyeHeight = SandboxMovementComponent->CrouchedHalfHeight * 0.80f;
	}
	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
	bIsRoadieRunning = false;
	bAddLocomotionBindings = true;
	BaseTurnRate = 45.0f;
	BaseLookUpRate = 45.0f;
	bIsMirrored = false;
	Health = 100.f;
	HealthMax = 120.f;

	//	Shaky Camera
	RoadieRunShakyCamDampeningFactor = 0.6f;
	CoverMovementPct[0] = 0.0f;
	CoverMovementPct[1] = 1.0f;
	CoverMovementPct[2] = 1.0f;

	WeaponSocket = TEXT("WeaponSocket");
	PelvisBoneName = TEXT("Pelvis");
	NeckBoneName = TEXT("neck_01");
	LeftWeaponBoneName = TEXT("LeftWeaponBone");
	RightWeaponBoneName= TEXT("RightWeaponBone");

	CoverAction = ECoverAction::CA_Default;
	CoverType = ECoverType::CT_None;
	CoverDirection = ECoverDirection::CD_Default;

	//	floor conform
	bDoMeshLocSmootherOffset = true;
	bCanDoFloorConform = true;
	bDoFloorConform = true;
	bDoFullMeshFloorConform = false;
	bDoIKFloorConformCOM = true;
	bDisableMeshTranslationChanges = false;
	bDoFootPlacement = true;
	bDoIKFloorConformTorsoBending = false;
	bUseFloorImpactNormal = false;
	FloorConformAverageFloorNormal = FVector::UpVector;
	FloorConformMaxSlopeAngle = 40.f;
	MeshFloorConformToggleAlpha.BlendTime = 0.42f;
	MeshFloorConformToggleAlpha.BlendType = EAlphaBlendType::ABT_Cubic;
	MeshFloorConformMovementAlpha.BlendType = EAlphaBlendType::ABT_Linear;
	IKFloorConformMovementAlpha.BlendType = EAlphaBlendType::ABT_Linear;
	IKFloorConformCOMLimits = FVector2D(25.f, 25.f);
	IKFloorConformCOMSpeed = FVector(50.f, 50.f, 50.f);
	IKFloorConformCOMDamping = FVector(10.f, 10.f, 10.f);
	IKFloorConformCOMWeight = 0.5f;
	FootPlacementAlpha.BlendTime = 0.42f;
	FootPlacementAlpha.BlendType = EAlphaBlendType::ABT_Cubic;
	LegAdjustInterpSpeedMoving = 36.f;
	LegAdjustInterpSpeedStopped = 128.f;
	IKFloorConformTorsoQuatWorld = FQuat::Identity;
	IKFloorConformTorsoQuatLocal = FQuat::Identity;
	IKFloorConformAlpha = 1.f;
	AlphaInterpSpeed = 12.f;
	MeshRotationSpeed = 90.f;
	IKFootLeftAlpha = 1.0f;
	IKFootRightAlpha = 1.0f;
	IKFootRightFloorNormal = FVector::UpVector;
	IKFootResetInterpSpeed = 12.0f;
	bDoIKFloorConformSkelControl = true;
	bDoLegFloorConformSkelControl = true;

	//	IK Hands
	bDoIKBoneCtrlRightHand = true;
	bDoIKBoneCtrlLeftHand = true;
	IKBoneCtrlLeftHandAlpha = 1.f;
	IKBoneCtrlRightHandAlpha = 1.f;

	// Step Smooth
	bCanDoStepsSmoothing = true;
	bAllowTurnSmoothing = true;
	TurningRadius = 32.f;
	bAllowAccelSmoothing = true;


	// Weapon fire
	AimTimeAfterFiring = 2.f;
	LastWeaponEndFireTime = -9999.f;
	LastWeaponStartFireTime = -9999.f;
	ForcedAimTime = -9999.f;
	LastShotAtTime = -9999.f;
	
	FiringMode = EFiringMode::FM_DEFAULT;
	bCombatSpeedModifier = true;

	bIsInCombat = false;

	RoadieRunTimer = 0.3f;
	bCanRoadieRun = true;
	bCanBeForcedToRoadieRun = true;
	bCanRoadieRunWhileTargeting = false;
	bCanBlindFire = true;
}

static FVector SavedRelativeLocation;
static FRotator SavedRelativeRotation;
static FVector SavedLocation;
static FRotator SavedRotation;
static bool SavedHardAttach;
static TEnumAsByte<ECoverType> SavedCoverType;

void ASandboxPawn::PreNetReceive()
{
	SavedLocation = GetActorLocation();
	SavedRotation = GetActorRotation();
	SavedCoverType = CoverType;
	Super::PreNetReceive();
}

void ASandboxPawn::PostNetReceive()
{
	SavedRelativeLocation = GetOwner()->GetRootComponent()->GetRelativeLocation();
	SavedRelativeRotation = GetOwner()->GetRootComponent()->GetRelativeRotation();

	Super::PostNetReceive();

	if (GetLocalRole() == ENetRole::ROLE_SimulatedProxy && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		if ((SavedLocation - GetActorLocation()).SizeSquared() < 2500.0f)
		{
			MTO_MeshLocSmootherOffset += SavedLocation - GetActorLocation();
		}
		if (bCanDoFloorConform)
		{
			float DeltaRot = FRotator::NormalizeAxis(SavedRotation.Yaw - GetActorRotation().Yaw);
			MeshYawOffset += DeltaRot;
		}
	}
}

void ASandboxPawn::MoveForward(float Value)
{
	aForward = Value;
	AddMovementInput(GetActorForwardVector() * aForward);
}

void ASandboxPawn::MoveRight(float Value)
{
	aStrafe = Value;
	if (!bIsRoadieRunning)
	{
		AddMovementInput(GetActorRightVector() * aStrafe);
	}
}

void ASandboxPawn::LookUp(float Value)
{
	aLookUp = Value;
	AddControllerPitchInput(aLookUp);
}

void ASandboxPawn::Turn(float Value)
{
	aTurn = Value;
	AddControllerYawInput(aTurn);
}

void ASandboxPawn::LookUpRate(float Value)
{
	aLookUp = Value;
	AddControllerPitchInput(Value * (BaseLookUpRate * GetWorld()->GetDeltaSeconds()));
}

void ASandboxPawn::TurnRate(float Value)
{
	aTurn = Value;
	AddControllerYawInput(Value * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASandboxPawn::ServerSetWalking_Implementation(bool bNewWalking)
{
	SetWalking(bNewWalking);
}

bool ASandboxPawn::ServerSetWalking_Validate(bool bNewWalking)
{
	return true;
}


void ASandboxPawn::SetWalking(bool bNewWalking)
{
	bIsWalking = bNewWalking;
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		ServerSetRoadieRun(bNewWalking);
	}
}

void ASandboxPawn::OnStartWalking()
{
	if (Controller)
	{
		SetWalking(true);
	}
}

void ASandboxPawn::OnStopWalking()
{
	if (Controller)
	{
		SetWalking(false);
	}
}

void ASandboxPawn::ServerSetRoadieRun_Implementation(bool bNewRoadieRun)
{
	SetRoadieRun(bNewRoadieRun);
}

bool ASandboxPawn::ServerSetRoadieRun_Validate(bool bNewRoadieRun)
{
	return true;
}

void ASandboxPawn::SetRoadieRun(bool bNewRoadieRun)
{
	bIsRoadieRunning = bNewRoadieRun;
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		ServerSetRoadieRun(bNewRoadieRun);
	}
}

void ASandboxPawn::TryToRoadieRun()
{
	ASandboxController* PC = Cast<ASandboxController>(Controller);
	if (PC->IsHoldingRoadieRunButton() && SandboxMovementComponent)
	{
		if (!IsTargeting() || !bIsCrouched || !bIsPOI || !bIsWalking)
		{
			SandboxMovementComponent->RoadieRunBoostTime = GetWorld()->GetTimeSeconds();
			SetRoadieRun(true);
			SandboxMovementComponent->RoadieRunMoveStarted();
		}
	}
}

void ASandboxPawn::OnStartRoadieRun()
{
	if (Controller)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_TryToRoadieRun, this, &ASandboxPawn::TryToRoadieRun, RoadieRunTimer);
	}
}

void ASandboxPawn::OnStopRoadieRun()
{
	if (Controller && SandboxMovementComponent)
	{
		SetRoadieRun(false);
	}
}


bool ASandboxPawn::ServerSetTargeting_Validate(bool bNewTargeting)
{
	return true;
}

void ASandboxPawn::ServerSetTargeting_Implementation(bool bNewTargeting)
{
	SetTargeting(bNewTargeting);
}

void ASandboxPawn::SetTargeting(bool bNewTargeting)
{
	bIsTargeting = bNewTargeting;
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		ServerSetTargeting(bNewTargeting);
	}
}

bool ASandboxPawn::IsReloadingWeapon() const
{
	return (FiringMode == EFiringMode::FM_RELOAD);
}

FVector ASandboxPawn::GetAimOffsetOrigin() const
{
	if (!GetMesh())
	{
		return GetActorLocation();
	}
	FVector VectorLoc(FVector::ZeroVector);
	if (WeaponSocket != NAME_None)
	{
		FVector SocketLoc(FVector::ZeroVector);
		FRotator SocketRot(FRotator::ZeroRotator);
		GetMesh()->GetSocketWorldLocationAndRotation(WeaponSocket, SocketLoc, SocketRot);
		FVector X, Y, Z;
		FRotationMatrix R(SocketRot);
		R.GetScaledAxes(X, Y, Z);
		return SocketLoc + Z * 12.f;
	}
	if (GetMesh() && PelvisBoneName != NAME_None)
	{
		VectorLoc = GetMesh()->GetBoneLocation(PelvisBoneName);
		VectorLoc.Z = GetPawnViewLocation().Z;

		return VectorLoc;
	}

	return GetActorLocation();
}

FRotator ASandboxPawn::GetAimOffsets() const
{
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(GetBaseAimRotation().Vector());
	const FRotator AimRotLS = AimDirLS.Rotation();
	return AimRotLS;
}

bool ASandboxPawn::IsInAimingPose()
{
	if (!IsReloadingWeapon())
	{
		if ((GetWorld()->GetTimeSeconds() < ForcedAimTime)
			|| ((GetWorld()->GetTimeSeconds() - LastWeaponEndFireTime) < AimTimeAfterFiring)
			|| IsTargeting())
		{
			return true;
		}
	}
	return false;
}

bool ASandboxPawn::IsTargeting() const
{
	return bIsTargeting;
}

void ASandboxPawn::OnStartTargeting()
{
	if (Controller)
	{
		SetTargeting(true);
	}
}

void ASandboxPawn::OnStopTargeting()
{
	if (Controller)
	{
		SetTargeting(false);
	}
}

void ASandboxPawn::StartFire()
{
	//nothing
}

void ASandboxPawn::StopFire()
{
	//nothing
}

void ASandboxPawn::SetPOI(bool bNewPOI)
{
	bIsPOI = bNewPOI;
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		// :(
	}
}

void ASandboxPawn::StartPOI()
{
	SetPOI(true);
}

void ASandboxPawn::StopPOI()
{
	SetPOI(false);
}

// Called when the game starts or when spawned
void ASandboxPawn::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentWeapon = GetWorld()->SpawnActor<ASandboxWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);
		}
	}
}


void ASandboxPawn::TickSpecial(float DeltaTime)
{
	if (bDoMeshLocSmootherOffset)
	{
		MeshLocSmootherOffset(DeltaTime);
	}

	if (bCanDoFloorConform)
	{
		UpdateFloorConform(DeltaTime);
		UpdateMeshTranslationOffset(MTO_MeshLocSmootherOffset, true);
	}

	if (SandboxMovementComponent)
	{
		if (bIsRoadieRunning)
		{
			SandboxMovementComponent->TickRoadieRun(DeltaTime);
		}
	}
}

// Called every frame
void ASandboxPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//	Special tick case
	TickSpecial(DeltaTime);
}

void InitSandboxPawnInputBindings()
{
	static bool bBindingsAdded = false;
	if (!bBindingsAdded)
	{
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_MoveForward", EKeys::W, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_MoveForward", EKeys::S, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_MoveForward", EKeys::Up, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_MoveForward", EKeys::Down, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_MoveForward", EKeys::Gamepad_LeftY, 1.f));

		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_MoveRight", EKeys::A, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_MoveRight", EKeys::D, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_MoveRight", EKeys::Gamepad_LeftX, 1.f));

		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_TurnRate", EKeys::Gamepad_RightX, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_TurnRate", EKeys::Left, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_TurnRate", EKeys::Right, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_Turn", EKeys::MouseX, 1.f));

		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_LookUpRate", EKeys::Gamepad_RightY, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("SandboxPawn_LookUp", EKeys::MouseY, -1.f));

		// Walk
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("SandboxPawn_Walk", EKeys::LeftControl));
		// Roadie-run
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("SandboxPawn_RoadieRun", EKeys::LeftShift));
		// Fire
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("SandboxPawn_StartFire", EKeys::LeftMouseButton));
		// Targeting
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("SandboxPawn_Targeting", EKeys::RightMouseButton));
		// POI
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("SandboxPawn_POI", EKeys::Q));

		bBindingsAdded = true;
	}
}

// Called to bind functionality to input
void ASandboxPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (bAddLocomotionBindings)
	{
		InitSandboxPawnInputBindings();
		PlayerInputComponent->BindAxis("SandboxPawn_MoveForward", this, &ASandboxPawn::MoveForward);
		PlayerInputComponent->BindAxis("SandboxPawn_MoveRight", this, &ASandboxPawn::MoveRight);

		PlayerInputComponent->BindAxis("SandboxPawn_LookUp", this, &ASandboxPawn::LookUp);
		PlayerInputComponent->BindAxis("SandboxPawn_LookUpRate", this, &ASandboxPawn::LookUpRate);
		PlayerInputComponent->BindAxis("SandboxPawn_Turn", this, &ASandboxPawn::Turn);
		PlayerInputComponent->BindAxis("SandboxPawn_TurnRate", this, &ASandboxPawn::TurnRate);


		PlayerInputComponent->BindAction("SandboxPawn_Walk", EInputEvent::IE_Pressed, this, &ASandboxPawn::OnStartWalking);
		PlayerInputComponent->BindAction("SandboxPawn_Walk", EInputEvent::IE_Released, this, &ASandboxPawn::OnStartWalking);

		PlayerInputComponent->BindAction("SandboxPawn_RoadieRun", EInputEvent::IE_Pressed, this, &ASandboxPawn::OnStartRoadieRun);
		PlayerInputComponent->BindAction("SandboxPawn_RoadieRun", EInputEvent::IE_Released, this, &ASandboxPawn::OnStopRoadieRun);

		PlayerInputComponent->BindAction("SandboxPawn_Targeting", EInputEvent::IE_Pressed, this, &ASandboxPawn::OnStartTargeting);
		PlayerInputComponent->BindAction("SandboxPawn_Targeting", EInputEvent::IE_Released, this, &ASandboxPawn::OnStopTargeting);

		PlayerInputComponent->BindAction("SandboxPawn_StartFire", EInputEvent::IE_Pressed, this, &ASandboxPawn::StartFire);
		PlayerInputComponent->BindAction("SandboxPawn_StartFire", EInputEvent::IE_Released, this, &ASandboxPawn::StopFire);

		PlayerInputComponent->BindAction("SandboxPawn_POI", EInputEvent::IE_Pressed, this, &ASandboxPawn::StartPOI);
		PlayerInputComponent->BindAction("SandboxPawn_POI", EInputEvent::IE_Released, this, &ASandboxPawn::StopPOI);
	}
}



bool ASandboxPawn::IsAliveAndWell() const
{
	return (Health > 0);
}

float ASandboxPawn::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (SandboxMovementComponent && SandboxMovementComponent->bAllowStoppingPower)
	{
		float InstigatorDistance = (EventInstigator->GetOwner()->GetActorLocation() - this->GetActorLocation()).Size();
		float CauserDistance = (DamageCauser->GetActorLocation() - this->GetActorLocation()).Size();
		float DmgDistance = InstigatorDistance ? InstigatorDistance : DamageCauser ? CauserDistance : 0.0f;
		SandboxMovementComponent->HandleStoppingPower(DamageEvent, this->GetActorLocation(), DmgDistance);
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

bool ASandboxPawn::PlayNearMissCringe()
{
	//this will play an animation slot
	return false;
}

bool ASandboxPawn::TryPlayNearMissCringe()
{
	if ((GetWorld()->GetTimeSeconds() - LastCringeTime) > 0.30f)
	{
		if (PlayNearMissCringe())
		{
			CringeHistory[CringeHistory.Num()] = GetWorld()->GetTimeSeconds();
			LastCringeTime = GetWorld()->GetTimeSeconds();
			return true;
		}
	}
	return false;
}


void ASandboxPawn::SetCurrentWeapon(ASandboxWeapon* NewWeapon, ASandboxWeapon* LastWeapon /*= NULL*/)
{
	ASandboxWeapon* LocalLastWeapon = NULL;
	if (LastWeapon != NULL)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}
	CurrentWeapon = NewWeapon;
}


void ASandboxPawn::OnRep_CurrentWeapon(ASandboxWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void ASandboxPawn::PerformStepsSmoothing(const FVector& OldLocation, float DeltaTime)
{
	// Stairs smoothing interpolation code
	if (GetMesh())
	{
		const FVector	CollisionStartLoc = GetCapsuleComponent()
			? (GetActorLocation() + GetCapsuleComponent()->GetComponentTransform().GetTranslation())
			: GetActorLocation();

		FVector LocationDelta = GetActorLocation() - OldLocation;
		float DeltaZ = LocationDelta.Z;
		if (DeltaZ < 0.f && GetMovementBase())
		{
			FCollisionQueryParams QueryParams(TEXT("ComputeDeltaZ"), true, this);
			FHitResult Hit;
			const FVector EndLocation = CollisionStartLoc + FVector(0.f, 0.f, -1.f * (GetCharacterMovement()->MaxStepHeight + MAXSTEPHEIGHTFUDGE));
			bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CollisionStartLoc, EndLocation, ECollisionChannel::ECC_Visibility, QueryParams);
			if (!Hit.bStartPenetrating && Hit.Normal.Z < 1.0f && Hit.Normal.Z >= GetCharacterMovement()->GetWalkableFloorZ())
			{
				float FloorDist = Hit.Time * (GetCharacterMovement()->MaxStepHeight + MAXSTEPHEIGHTFUDGE);
				if (FloorDist < MINFLOORDIST)
				{
					FVector NewMove = FVector(0.f, 0.f, 0.5f * (MINFLOORDIST + MAXFLOORDIST) - FloorDist);
					SetActorLocation(NewMove, false, &Hit);
				}
				DeltaZ = GetActorLocation().Z - OldLocation.Z;
			}
		}
		bool bShouldPerformStepsSmoothing = bCanDoStepsSmoothing
			&& ((GetCharacterMovement()->GetGroundMovementMode() == EMovementMode::MOVE_Falling)
				|| (GetCharacterMovement()->GetGroundMovementMode() == EMovementMode::MOVE_Walking)
				|| (GetCharacterMovement()->GetGroundMovementMode() == EMovementMode::MOVE_Custom))
			&& !GetVelocity().IsNearlyZero();

		if (bShouldPerformStepsSmoothing && GetCharacterMovement()->GetGroundMovementMode() == EMovementMode::MOVE_Falling)
		{
			const FVector CheckVector(0.f, 0.f, -GetCharacterMovement()->MaxStepHeight);
			FCollisionQueryParams QueryParams(TEXT("ComputeDeltaZ"), true, this);
			FHitResult Hit;
			bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CollisionStartLoc, CollisionStartLoc + CheckVector, ECollisionChannel::ECC_WorldStatic, QueryParams);
			if (!Hit.GetActor())
			{
				bShouldPerformStepsSmoothing = false;
			}
		}
		if (bShouldPerformStepsSmoothing && DeltaZ != 0.f)
		{
			float OffsetZ = MTO_MeshLocSmootherOffset.Z - DeltaZ;
			float OffsetZMax = GetCharacterMovement()->MaxStepHeight * 1.5f;
			MTO_MeshLocSmootherOffset.Z = FMath::Clamp<float>(OffsetZ, OffsetZMax*(-1.f), OffsetZMax);
		}

		if (GetLocalRole() == ENetRole::ROLE_Authority
			&& GetController() != nullptr
			&& !IsLocallyControlled()
			&& GetCharacterMovement()->GetGroundMovementMode() == EMovementMode::MOVE_Walking)
		{
			LocationDelta.Z = 0.f;
			const float DeltaSize = LocationDelta.Size();
			if (DeltaSize > 0 && DeltaSize < GetCharacterMovement()->GetMaxSpeed())
			{
				MTO_MeshLocSmootherOffset -= LocationDelta;
			}
		}
	}
}

bool ASandboxPawn::GetFloorConformTranslationAndAngle(FVector &InFloorNormal, float &OutTranslation, float &OutSlopeAngle, FVector &OutFloorNormal2D)
{
	float AABBRadius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	bool bStraightUp = (InFloorNormal.Z >= 1.f - SMALL_NUMBER);
	OutFloorNormal2D = FVector::ZeroVector;
	if (!bStraightUp)
	{
		OutFloorNormal2D = InFloorNormal.GetSafeNormal2D();
		if (OutFloorNormal2D.IsZero())
		{
			bStraightUp = true;
		}
		if (!bStraightUp)
		{
			float Adjacent = FMath::Max<float>(FMath::Abs(OutFloorNormal2D.X), FMath::Abs(OutFloorNormal2D.Y));
			if (Adjacent > KINDA_SMALL_NUMBER)
			{
				AABBRadius /= Adjacent;
			}

			OutSlopeAngle = FMath::Max(FMath::Acos(InFloorNormal.Z), 0.f);
			OutTranslation = FMath::Tan(OutSlopeAngle) * AABBRadius;
		}
	}
	return bStraightUp;
}

void ASandboxPawn::UpdateNormalInterpolation(FTAlphaBlend& AlphaInterpolator, FVector& CurrentNormal, const FVector& TargetNormal, FVector& BackupNormal, float DeltaTime, float InterpolationSpeed)
{
	const float Speed2D = GetVelocity().SizeSquared2D();
	const bool bDoLazyInterpolation = (Speed2D > 1.f);

	if (bDoLazyInterpolation)
	{
		float const SpeedScale = FMath::Clamp<float>(Speed2D / FMath::Square<float>(GetCharacterMovement()->GetMaxSpeed()), 0.75f, 1.5f);
		InterpolationSpeed *= SpeedScale;
		AlphaInterpolator.Reset();
		CurrentNormal = FMath::VInterpConstantTo(CurrentNormal, TargetNormal, DeltaTime, InterpolationSpeed).GetSafeNormal();
	}
	else
	{
		if (!AlphaInterpolator.GetToggleStatus())
		{
			BackupNormal = CurrentNormal;
			const FVector Delta = TargetNormal - BackupNormal;
			const float DeltaM = Delta.Size();
			const float BlendTime = DeltaM / InterpolationSpeed;
			AlphaInterpolator.SetBlendTime(BlendTime);
			AlphaInterpolator.Toggle(true);
		}

		AlphaInterpolator.Update(DeltaTime);

		if (AlphaInterpolator.AlphaOut < 1.f - KINDA_SMALL_NUMBER)
		{
			const FVector TempNormal = FMath::Lerp<FVector>(BackupNormal, TargetNormal, AlphaInterpolator.AlphaOut);
			CurrentNormal = TempNormal.GetSafeNormal();
		}
		else
		{
			CurrentNormal = TargetNormal;
		}
	}
}

FVector ASandboxPawn::GetFloorConformNormal()
{
	if (GetMovementBase() && GetMovementBase()->IsWorldGeometry())
	{
		ASandboxGameModeBase* SandGame = Cast<ASandboxGameModeBase>(GetWorld()->GetGameInstance());
		if (SandGame && SandGame->GetIsReplicated())
		{
			float const CurrentTime = GetWorld()->GetTimeSeconds();
			if (SandGame->FloorConformTimeStamp != CurrentTime)
			{
				SandGame->FloorConformTimeStamp = CurrentTime;
				if (SandGame->NumFloorConformUpdatesThisFrame < SandGame->MaxFloorConformUpdatesPerFrame)
				{
					SandGame->FloorConformUpdateTag++;
				}
				SandGame->NumFloorConformUpdatesThisFrame = 0;
			}
			bool const bDoUpdate = (SandGame->NumFloorConformUpdatesThisFrame < SandGame->MaxFloorConformUpdatesPerFrame) && (FloorConformUpdateTag != SandGame->FloorConformUpdateTag);
			if (!bDoUpdate)
			{
				return FloorConformAverageFloorNormal;
			}

			SandGame->NumFloorConformUpdatesThisFrame++;
			FloorConformUpdateTag = SandGame->FloorConformUpdateTag;
		}
		if (GetCharacterMovement() && GetCharacterMovement()->CurrentFloor.IsWalkableFloor())
		{
			FloorConformAverageFloorNormal = GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;
		}
		else
		{
			FloorConformAverageFloorNormal = FVector::UpVector;
		}
	}
	else
	{
		FloorConformAverageFloorNormal = FVector::UpVector;
	}

	return FloorConformAverageFloorNormal;
}

void ASandboxPawn::UpdateFloorConform(float DeltaTime)
{
	checkSlow(GetCapsuleComponent() && GetMesh());
	const ASandboxPawn* DefaultPawn = Cast<ASandboxPawn>(GetClass()->GetDefaultObject());
	FMatrix const LocalToWorldTM = GetTransform().ToMatrixWithScale();
	FMatrix const WorldToLocalTM = LocalToWorldTM.InverseFast();
	const FRotationMatrix PawnRM(GetActorRotation());
	const bool bPawnRotationChanged = (GetActorRotation() != FloorConformLastPawnRotation);
	FloorConformLastPawnRotation = GetActorRotation();
	FloorConformTranslation = FVector::ZeroVector;
	FRotator TorsoBendingRotation(FRotator::ZeroRotator);
	FVector CurrentFloorNormal = bUseFloorImpactNormal ? GetFloorConformNormal() : FVector::UpVector;

	if (bDebugFloorConform)
	{
#if WITH_EDITOR
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + CurrentFloorNormal * 100.f, FColor::Red, false, 0.f, 1.f);
#endif
	}
	if (!GetVelocity().IsNearlyZero())
	{
		FVector CurrentFloorNormal2D = CurrentFloorNormal.GetSafeNormal2D();
		FVector CurrentSlopeAxisX(FVector::ZeroVector);
		if (!CurrentFloorNormal2D.IsZero())
		{
			const FVector CTempY = CurrentFloorNormal2D ^ FVector::UpVector;
			CurrentSlopeAxisX = CTempY ^ CurrentFloorNormal;
		}
		else
		{
			CurrentSlopeAxisX = PawnRM.GetScaledAxis(EAxis::X);
		}
		FVector SlopeVelocity = (GetVelocity() | CurrentSlopeAxisX) * CurrentSlopeAxisX;
		MTO_MeshLocSmootherOffset.Z += SlopeVelocity.Z * DeltaTime;
	}

	if (CurrentFloorNormal.Z < 0.9f)
	{
		FQuat DeltaQuat = FQuat::FindBetween(FVector::UpVector, CurrentFloorNormal);
		FVector DeltaAxis(FVector::ZeroVector);
		float DeltaAngle = 0.f;
		DeltaQuat.ToAxisAndAngle(DeltaAxis, DeltaAngle);
		const float MaxRotRadians = FloorConformMaxSlopeAngle * (PI / 180.f);
		if (FMath::Abs(DeltaAngle) > MaxRotRadians)
		{
			DeltaAngle = FMath::Clamp<float>(DeltaAngle, -MaxRotRadians, MaxRotRadians);
			DeltaQuat = FQuat(DeltaAxis, DeltaAngle);
			CurrentFloorNormal = FQuatRotationTranslationMatrix(DeltaQuat, FVector::ZeroVector).TransformVector(FVector::UpVector);
		}
	}

	bool const bSkipFootPlacement = !IsAliveAndWell();
	bool const bDoingMeshFloorConform = !bSkipFootPlacement && bDoFloorConform && !bDisableMeshTranslationChanges && (bDoFullMeshFloorConform);
	bool const bDoingIKFloorConform = !bSkipFootPlacement && bDoFloorConform && !bDisableMeshTranslationChanges && !bDoingMeshFloorConform;
	bool const bShouldDoFootPlacement = !bSkipFootPlacement && bDoFootPlacement && !bDisableMeshTranslationChanges;

	if (bDebugFloorConform)
	{
#if WITH_EDITOR
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::White, FString::Printf(TEXT("bDoingMeshFloorConform: %d, bDoingIKFloorConform: %d, bShouldDoFootPlacement: %d"),
			bDoingMeshFloorConform, bDoingIKFloorConform, bShouldDoFootPlacement));
#endif
	}

	MeshFloorConformToggleAlpha.Toggle(bDoingMeshFloorConform);
	MeshFloorConformToggleAlpha.Update(DeltaTime);
	const FVector TargetMeshFloorNormal = FMath::Lerp<FVector>(FVector::UpVector, CurrentFloorNormal, MeshFloorConformToggleAlpha.AlphaOut).GetSafeNormal();
	if ((bPawnRotationChanged && MeshFloorConformToggleAlpha.AlphaOut > 0.f) || MeshFloorConformNormal != TargetMeshFloorNormal)
	{
		UpdateNormalInterpolation(MeshFloorConformMovementAlpha, MeshFloorConformNormal, TargetMeshFloorNormal, MeshFloorConformBackupNormal, DeltaTime, 1.f);
		FVector MeshFloorConformNormal2D;
		GetFloorConformTranslationAndAngle(MeshFloorConformNormal, MeshFloorConformTranslation, MeshFloorConformSlopeAngle, MeshFloorConformNormal2D);
		FQuat DeltaQuat = FQuat::FindBetween(FVector::UpVector, MeshFloorConformNormal);
		FVector DeltaAxis(FVector::ZeroVector);
		float DeltaAngle = 0.f;
		DeltaQuat.ToAxisAndAngle(DeltaAxis, DeltaAngle);
		FVector PawnSpaceAxis = PawnRM.InverseTransformVector(DeltaAxis);
		MeshFloorConformRotation = FQuatRotationTranslationMatrix(FQuat(PawnSpaceAxis, DeltaAngle), FVector::ZeroVector).Rotator();
	}

	bool const bRotatingBaseCompensation = false;
	if (bDoIKFloorConformSkelControl)
	{
		bool const bIKFloorSkelControlActive = (IKFloorConformAlpha > 0.f);
		if (bIKFloorSkelControlActive != bDoingIKFloorConform)
		{
			IKFloorConformAlpha = bDoingIKFloorConform
				? FMath::FInterpTo(IKFloorConformAlpha, 1.f, DeltaTime, AlphaInterpSpeed)
				: FMath::FInterpTo(IKFloorConformAlpha, 0.f, DeltaTime, AlphaInterpSpeed);
		}

		float const Alpha = IKFloorConformAlpha;
		FVector const TargetIKFloorNormal = FMath::Lerp<FVector>(FVector::UpVector, CurrentFloorNormal, Alpha).GetSafeNormal();
		bool bIKFloorNormalUpdated = (IKFloorConformNormal != TargetIKFloorNormal);
		if ((bPawnRotationChanged && Alpha > 0.f) || bIKFloorNormalUpdated)
		{
			FVector OldNormal = IKFloorConformNormal;
			UpdateNormalInterpolation(IKFloorConformMovementAlpha, IKFloorConformNormal, TargetIKFloorNormal, IKFloorConformBackupNormal, DeltaTime, 1.f);
			FQuat DeltaQuat = FQuat::FindBetween(PawnRM.GetScaledAxis(EAxis::Z), IKFloorConformNormal);
			IKFloorConformBoneRot = FQuatRotationTranslationMatrix(DeltaQuat, FVector::ZeroVector).Rotator();
			if (bDoLegFloorConformSkelControl)
			{
				LegFloorConformBoneRot = IKFloorConformBoneRot;
			}
			if (!bRotatingBaseCompensation && bDoIKFloorConformTorsoBending)
			{
				DeltaQuat = FQuat::FindBetween(IKFloorConformNormal, OldNormal);
				IKFloorConformTorsoQuatWorld = DeltaQuat * IKFloorConformTorsoQuatWorld;
			}
		}
		FVector SlopeAxisX = PawnRM.GetScaledAxis(EAxis::X);
		FVector SlopeAxisY = PawnRM.GetScaledAxis(EAxis::Y);
		FVector IKFloorNormal2D(FVector::ZeroVector);
		bool bStraightUp = GetFloorConformTranslationAndAngle(IKFloorConformNormal, IKFloorConformTranslation, IKFloorConformSlopeAngle, IKFloorNormal2D);
		FVector NewOriginLoc(FVector::ZeroVector);
		float const Hypotenuse = IKFloorConformTranslation - DefaultPawn->GetMesh()->GetComponentTransform().GetTranslation().Z;
		float const Adjacent = FMath::Cos(IKFloorConformSlopeAngle) * Hypotenuse;
		const float XLocal = DefaultPawn->GetMesh()->GetComponentTransform().GetTranslation().X;
		const float YLocal = DefaultPawn->GetMesh()->GetComponentTransform().GetTranslation().Y;
		FVector const DefaultMeshXYLocal(XLocal, YLocal, 0.f);
		FVector const DefaultMeshXYWorld = LocalToWorldTM.TransformVector(DefaultMeshXYWorld);
		FVector const MTO_MeshLocSmootherOffsetV(MTO_MeshLocSmootherOffset.X, MTO_MeshLocSmootherOffset.Y, MTO_MeshLocSmootherOffset.Z);
		FVector const PawnBaseLocation = GetActorLocation() + MTO_MeshLocSmootherOffsetV + DefaultMeshXYWorld;
		if (!bStraightUp)
		{
			FVector const TempY = IKFloorNormal2D ^ FVector::UpVector;
			SlopeAxisX = TempY ^ IKFloorConformNormal;
			SlopeAxisY = IKFloorConformNormal ^ SlopeAxisX;
			NewOriginLoc = PawnBaseLocation - IKFloorConformNormal * Adjacent;
			IKFloorConformTranslation = Adjacent + DefaultPawn->GetMesh()->GetComponentTransform().GetTranslation().Z;
		}
		else
		{
			NewOriginLoc = PawnBaseLocation - IKFloorConformNormal * Hypotenuse;
			IKFloorConformTranslation = 0.f;
		}

		IKFloorConformBoneLoc = WorldToLocalTM.TransformPosition(NewOriginLoc);
		if (bDoIKFloorConformCOM)
		{
			float const COMWeightTarget = (GetVelocity().SizeSquared2D() > KINDA_SMALL_NUMBER) ? 0.33f : 0.5f;
			IKFloorConformCOMWeight = FMath::FInterpConstantTo(IKFloorConformCOMWeight, COMWeightTarget, DeltaTime, 2.f);
			if (bDoLegFloorConformSkelControl)
			{
				LegFloorConformBoneRot = (IKFloorConformBoneRot * IKFloorConformCOMWeight);
			}
			FVector const HypoVector(0, 0, Hypotenuse);
			FVector COMTargetActorSpace = (IKFloorConformBoneLoc + HypoVector) * IKFloorConformCOMWeight;
			if (!bRotatingBaseCompensation)
			{
				IKFloorConformBoneLoc -= COMTargetActorSpace;
				IKFloorConformTranslation = DefaultPawn->GetMesh()->GetComponentTransform().GetTranslation().Y + FMath::Lerp<float>(Adjacent, Hypotenuse, IKFloorConformCOMWeight);
			}
			else
			{
				COMTargetActorSpace.X = FMath::Clamp<float>(COMTargetActorSpace.X, -IKFloorConformCOMLimits.X, IKFloorConformCOMLimits.X);
				COMTargetActorSpace.Y = FMath::Clamp<float>(COMTargetActorSpace.Y, -IKFloorConformCOMLimits.X, IKFloorConformCOMLimits.X);
				COMTargetActorSpace.Z = FMath::Clamp<float>(COMTargetActorSpace.Z, -IKFloorConformCOMLimits.Y, IKFloorConformCOMLimits.Y);
				FRotationMatrix const WorldToSlopeTM(IKFloorConformBoneRot);
				IKFloorConformCOMTarget = COMTargetActorSpace;
				IKFloorConformCOMTimeAccumulator += DeltaTime;
				const float SubStep = 1 / 60.f;
				while (IKFloorConformCOMTimeAccumulator > SubStep)
				{
					IKFloorConformCOMTimeAccumulator -= SubStep;
					FVector COMDelta = IKFloorConformCOMTarget - IKFloorConformCOMTranslation;
					if (!COMDelta.IsNearlyZero())
					{
						const FVector DampingForce = IKFloorConformCOMDamping * IKFloorConformCOMVelocity;
						const FVector SpringForce = IKFloorConformCOMSpeed * COMDelta;
						const FVector Acceleration = SpringForce - DampingForce;
						IKFloorConformCOMVelocity += Acceleration * SubStep;
						const FVector DeltaMove = IKFloorConformCOMVelocity * SubStep;
						IKFloorConformCOMTranslation += DeltaMove;
					}
				}
				FloorConformTranslation += IKFloorConformCOMTranslation;
			}
		}

		if (FMath::Square<float>(IKFloorConformTorsoQuatWorld.W) < 1.f - DELTA * DELTA)
		{
			FVector DeltaAxis(FVector::ZeroVector);
			float DeltaAngle = 0.f;
			IKFloorConformTorsoQuatWorld.ToAxisAndAngle(DeltaAxis, DeltaAngle);
			DeltaAngle = FMath::FInterpTo(DeltaAngle, 0.f, DeltaTime, 8.f);
			IKFloorConformTorsoQuatWorld = FQuat(DeltaAxis, DeltaAngle);
			FVector const PawnSpaceAxis = PawnRM.InverseTransformVector(DeltaAxis);
			IKFloorConformTorsoQuatLocal = FQuat(PawnSpaceAxis, DeltaAngle);
			TorsoBendingRotation = FQuatRotationTranslationMatrix(IKFloorConformTorsoQuatLocal, FVector::ZeroVector).Rotator();
			FVector const TorsoUpLocal = WorldToLocalTM.TransformVector(FVector::UpVector * (-DefaultPawn->GetMesh()->GetComponentTransform().GetTranslation()));
			FVector const TranslationOffset = (TorsoUpLocal - IKFloorConformTorsoQuatLocal.RotateVector(TorsoUpLocal));
			FloorConformTranslation += TranslationOffset;
		}
	}

	if (USandboxAnimation* AnimInst = Cast<USandboxAnimation>(GetMesh()->GetAnimInstance()))
	{
		FVector IKFloorConformFeetPlacementMeshAdjust(FVector::ZeroVector);
		FootPlacementAlpha.Toggle(bShouldDoFootPlacement);
		FootPlacementAlpha.Update(DeltaTime);
		IKFootLeftAlpha = FootPlacementAlpha.AlphaOut;
		IKFootRightAlpha = FootPlacementAlpha.AlphaOut;
		IKFootLeftFloorNormal = FVector::UpVector;
		IKFootRightFloorNormal = FVector::UpVector;
		float const TargetAdjust = CoverType == ECoverType::CT_MidLevel
			? FMath::Max<float>(-AnimInst->IKFootLeftLegAdjust, -AnimInst->IKFootRightLegAdjust)
			: FMath::Min<float>(-AnimInst->IKFootLeftLegAdjust, -AnimInst->IKFootRightLegAdjust);

		float const InterpSpeed = (GetVelocity().SizeSquared2D() > KINDA_SMALL_NUMBER) ? LegAdjustInterpSpeedMoving : LegAdjustInterpSpeedStopped;
		LegAdjust = FMath::FInterpConstantTo(LegAdjust, TargetAdjust, DeltaTime, InterpSpeed);
		FootPlacementLegAdjust = FVector(0, 0, -LegAdjust);
		IKFootResetInterpSpeed = InterpSpeed;
		if (FMath::Square<float>(LegAdjust) > KINDA_SMALL_NUMBER)
		{
			IKFloorConformFeetPlacementMeshAdjust = WorldToLocalTM.TransformVector(CurrentFloorNormal * LegAdjust);
		}

		float NewZTranslation = FMath::Max<float>(MeshFloorConformTranslation, IKFloorConformTranslation);
		NewZTranslation = FMath::Clamp<float>(NewZTranslation, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f);
		float DeltaZTranslation = NewZTranslation - FloorConformZTranslation;
		FloorConformZTranslation = NewZTranslation;
		FloorConformTranslation.Z -= NewZTranslation;
		FloorConformTranslation += IKFloorConformFeetPlacementMeshAdjust;

		FRotator const NewMeshRotation = (MeshFloorConformRotation + TorsoBendingRotation).GetNormalized();
		GetMesh()->AddRelativeRotation(NewMeshRotation);
		if (FMath::Abs(MeshYawOffset) > KINDA_SMALL_NUMBER && !bDisableMeshTranslationChanges)
		{
			MeshYawOffset = FMath::FInterpConstantTo(MeshYawOffset, 0.f, DeltaTime, MeshRotationSpeed);
			FRotator CurrentRotation = GetMesh()->GetRelativeRotation();
			CurrentRotation.Yaw += MeshYawOffset;
			GetMesh()->AddRelativeRotation(CurrentRotation);
		}
	}
}

bool ASandboxPawn::UpdateMeshTranslationOffset(FVector NewOffset, bool bForce /*= false*/)
{
	if ((bForce || (MTO_MeshLocSmootherOffset != NewOffset)) && !bDisableMeshTranslationChanges)
	{
		MTO_MeshLocSmootherOffset = NewOffset;
		const ASandboxPawn* DefaultPawn = Cast<ASandboxPawn>(GetClass()->GetDefaultObject());
		if (GetMesh() && (DefaultPawn && DefaultPawn->GetMesh()))
		{
			FVector const LocalOffset = GetTransform().ToMatrixNoScale().InverseTransformVector(MTO_MeshLocSmootherOffset + MTO_SpecialMoveInterp);
			GetMesh()->SetRelativeLocation(DefaultPawn->GetMesh()->GetRelativeLocation() + FloorConformTranslation + LocalOffset);
			//BaseTranslationOffset.Z = GetMesh()->GetRelativeLocation().Z;
		}
	}
	return true;
}

void ASandboxPawn::MeshLocSmootherOffset(float DeltaTime)
{
	float InterpAmt = (GetLocalRole() == ENetRole::ROLE_SimulatedProxy) ? 1.f : 40.f;

	// new mesh trans offset
	FVector NewMTO_MeshLocSmootherOffset(FVector::ZeroVector);

	// First apply in-game offset - interpolate
	// if special move offset is set or it is being interpolated 
	bool bShouldDoSpecialMoveOffsetInterp = !MTO_SpecialMoveOffset.IsZero() || !MTO_SpecialMoveInterp.IsZero();
	if (bShouldDoSpecialMoveOffsetInterp)
	{
		// recalculate interpolated vector
		MTO_SpecialMoveInterp = FMath::VInterpTo(MTO_SpecialMoveInterp, MTO_SpecialMoveOffset, DeltaTime, MTO_SpecialMoveSpeed);
		NewMTO_MeshLocSmootherOffset = MTO_MeshLocSmootherOffset;
	}

	// check to see if we should interpolate the Mesh->Translation.Z from the previous frame
	bool bShouldDoMeshTranslation = (MTO_MeshLocSmootherOffset.SizeSquared() != 0.f);
	if (bShouldDoMeshTranslation)
	{
		// if this is a remote pawn that is still moving
		if (!IsLocallyControlled() && GetVelocity().SizeSquared() > KINDA_SMALL_NUMBER)
		{
			// scale the interpolation amount down if moving in the same direction as the offset
			const float InterpScale = 1.f - (GetVelocity().GetSafeNormal() | (MTO_MeshLocSmootherOffset.GetSafeNormal() * -1));
			InterpAmt *= InterpScale;
		}
		NewMTO_MeshLocSmootherOffset.X = FMath::FInterpTo(MTO_MeshLocSmootherOffset.X, 0.f, DeltaTime, FMath::Max<float>(InterpAmt * FMath::Abs(MTO_MeshLocSmootherOffset.X) / MAX_MESHTRANSLATIONOFFSET, MIN_MESHTRANSLATIONOFFSET));
		NewMTO_MeshLocSmootherOffset.Y = FMath::FInterpTo(MTO_MeshLocSmootherOffset.Y, 0.f, DeltaTime, FMath::Max<float>(InterpAmt * FMath::Abs(MTO_MeshLocSmootherOffset.Y) / MAX_MESHTRANSLATIONOFFSET, MIN_MESHTRANSLATIONOFFSET));
		NewMTO_MeshLocSmootherOffset.Z = FMath::FInterpTo(MTO_MeshLocSmootherOffset.Z, 0.f, DeltaTime, FMath::Max<float>(InterpAmt * FMath::Abs(MTO_MeshLocSmootherOffset.Z) / MAX_MESHTRANSLATIONOFFSET, MIN_MESHTRANSLATIONOFFSET));
	}

	// if either special move offset or mesh translation, you need to call UpdateMeshTranslationOffset to accumulate the result
	if (bShouldDoSpecialMoveOffsetInterp || bShouldDoMeshTranslation)
	{
		UpdateMeshTranslationOffset(NewMTO_MeshLocSmootherOffset);
	}
}

bool ASandboxPawn::IsEvading() const
{
	return false;
}

/** REPLICATION WIP*/
void ASandboxPawn::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ASandboxPawn, CoverType, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASandboxPawn, CoverAction, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASandboxPawn, bIsRoadieRunning, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASandboxPawn, bIsTargeting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASandboxPawn, CurrentWeapon, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASandboxPawn, bIsWalking, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASandboxPawn, bIsPOI, COND_SkipOwner);
	DOREPLIFETIME(ASandboxPawn, bIsInCombat);
	DOREPLIFETIME(ASandboxPawn, WeaponInventory);
	DOREPLIFETIME(ASandboxPawn, FiringMode);
}

