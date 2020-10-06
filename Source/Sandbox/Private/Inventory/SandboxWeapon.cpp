
#include "SandboxWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "SandboxPawn.h"
#include "Net/UnrealNetwork.h"

ASandboxWeapon::ASandboxWeapon()
{
	SkelMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkelMeshComp"));
	RootComponent = SkelMeshComp;

	SetReplicates(true);

	ShootDirectionMode = EWeaponShootDirection::WSD_EyeLocation;

	MuzzleName = "MuzzleSocket";
	TracerName = "Target";

	BaseDamage = 51.f;
	WeaponRateOfFire = 550.f;
	bLoopFire = true;
	TraceEndDepth = 10000;
	WeaponMagSize = 60.f;
	WeaponRecoil = 35.f;
	WeaponAimError = FVector2D(1.5, 5);
	WeaponMaxSpareAmmo = 450.f;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	bAllowDownsightsStance = true;
	WeaponRange = 16384.f;

	TargetingFOV = 45.f;

	bUseTargetingCamera = true;
	bIronSightsCamera = false;
	bSuperSteadyCamWhileTargeting = false;
	bTemporaryHolster = false;
	bAllowsRoadieRunning = true;
	bSupports360AimingInCover = true;
	bAllowIdleStance = true;
	bAllowAimingStance = true;
	bAllowDownsightsStance = true;
	WeaponFireTypes.Empty();
	WeaponFireTypes.SetNumUninitialized(12);
	FiringStatesArray.Empty();
	FiringStatesArray.SetNumUninitialized(12);
	PendingFire.Empty();
	PendingFire.SetNumUninitialized(12);
	bUseHeavyWeaponSpeedMods = false;
	WeaponType = EWeaponType::WT_Normal;
	FiringStatesArray[EFiringMode::FM_DEFAULT] = EWeaponState::EWS_WeaponFiring;
	WeaponFireTypes[EFiringMode::FM_DEFAULT] = EWeaponFireType::EWFT_InstantHit;
	FiringStatesArray[EFiringMode::FM_RELOAD] = EWeaponState::EWS_Reloading;
	WeaponFireTypes[EFiringMode::FM_RELOAD] = EWeaponFireType::EWFT_InstantHit;
	FiringStatesArray[EFiringMode::FM_MELEEATTACK] = EWeaponState::EWS_MeleeAttacking;
	WeaponFireTypes[EFiringMode::FM_MELEEATTACK] = EWeaponFireType::EWFT_InstantHit;
}

void ASandboxWeapon::OnRep_HitScanTrace()
{
	//
}

bool ASandboxWeapon::ShouldTargetingModeZoomCamera()
{
	if (ASandboxPawn* InstigatorGP = Cast<ASandboxPawn>(GetOwner()))
	{
		return (bUseTargetingCamera && !IsReloading() && InstigatorGP->IsTargeting());
	}
	return false;
}

bool ASandboxWeapon::IsReloading()
{
	if (ASandboxPawn* InstigatorGP = Cast<ASandboxPawn>(GetOwner()))
	{
		return InstigatorGP->IsReloadingWeapon();
	}
	return false;
}

float ASandboxWeapon::GetAdjustedFOV(float BaseFOV)
{
	return BaseFOV;
}

float ASandboxWeapon::GetTargetingFOV(float BaseFOV)
{
	if (TargetingFOV <= 0)
	{
		return BaseFOV;
	}
	else
	{
		return TargetingFOV;
	}
}

// NETWORK
void ASandboxWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASandboxWeapon, HitScanTrace, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASandboxWeapon, AmmoUsedCount, COND_SkipOwner);

}

