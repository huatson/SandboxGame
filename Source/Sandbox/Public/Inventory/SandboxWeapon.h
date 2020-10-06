#pragma once

#include "CoreMinimal.h"
#include "SandboxWeaponTypes.h"
#include "GameFramework/Actor.h"
#include "SandboxWeapon.generated.h"

class ASandboxPawn;
class ASandboxController;
class USkeletalMeshComponent;
class UDamageType;
class UParticleSystem;
class UCameraShake;

UCLASS(Abstract, Blueprintable)
class SANDBOX_API ASandboxWeapon : public AActor
{
	GENERATED_BODY()

public:
	ASandboxWeapon();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	USkeletalMeshComponent* SkelMeshComp;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TEnumAsByte<EWeaponShootDirection> ShootDirectionMode;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8	bSupports360AimingInCover : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8	bAllowsRoadieRunning : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8	bAllowIdleStance : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8	bAllowAimingStance : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8 bAllowDownsightsStance : 1;
	uint8	bTemporaryHolster : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8	bUseTargetingCamera : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8	bCanDoIronSights : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8	bIronSightsCamera : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector IronSightsCameraOffset;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FRotator IronSightsCameraRotOffset;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8 bSuperSteadyCamWhileTargeting : 1;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleName;
	virtual bool DisableAimOffset() { return false; }
	uint8 bUseHeavyWeaponSpeedMods : 1;

private:
	/**
	 * Main weapon internal settings
	 */
	TEnumAsByte<EWeaponType>				WeaponType;
	TEnumAsByte<EFiringMode>				PreviousFiringMode;
	TEnumAsByte<EFiringMode>				CurrentFireMode;
	TArray<int32>							PendingFire;
	TArray<TEnumAsByte<EWeaponState>>		FiringStatesArray;
	TEnumAsByte<EWeaponState>				CurrentState;
	TArray<TEnumAsByte<EWeaponFireType>>	WeaponFireTypes;

protected:

	TEnumAsByte<EFiringMode> ServerFireModeNum;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerFX;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* DefaultImpactFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* FleshImpactFX;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponMagSize;

	UPROPERTY(Replicated)
	uint32		AmmoUsedCount;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponMaxSpareAmmo;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float			WeaponRecoil;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector2D		WeaponAimError;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponRateOfFire;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint32 TraceEndDepth;

	FTimerHandle TH_TimeBetweenShots;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8 bLoopFire : 1;

	UFUNCTION()
	void OnRep_HitScanTrace();

	UPROPERTY(ReplicatedUsing = OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	float	TargetingFOV;
	FVector ShootDirection;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Firing")
	float	WeaponRange;
	UPROPERTY(EditDefaultsOnly, Category = "Firing")
	float	EquipTime;
	UPROPERTY(EditDefaultsOnly, Category = "Firing")
	float	PutDownTime;
	UPROPERTY(EditDefaultsOnly, Category = "Firing")
	FVector FireOffset;
	UPROPERTY(EditDefaultsOnly, Category = "Firing")
	uint8	bWeaponPutDown : 1;
	FTimerHandle TH_RefireCheckTimer;
	FTimerHandle TH_EndOfReloadTimer;


public:

	bool ShouldTargetingModeZoomCamera();
	bool IsReloading();
	float GetAdjustedFOV(float BaseFOV);
	float GetTargetingFOV(float BaseFOV);
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return SkelMeshComp; }
};