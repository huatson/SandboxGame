#pragma once

#include "CoreMinimal.h"
#include "SandboxWeaponTypes.generated.h"


UENUM(BlueprintType)
enum EWeaponFireType
{
	EWFT_InstantHit,	
	EWFT_Projectile,	
	EWFT_Custom,		
	EWFT_None
};

UENUM(BlueprintType)
enum EWeaponState
{
	EWS_Active,
	EWS_WeaponFiring,
	EWS_Reloading,
	EWS_MeleeAttacking,
	EWS_WeaponEquipping,
	EWS_WeaponPuttingDown
};

UENUM(BlueprintType)
enum EWeaponShootDirection
{
	WSD_EyeLocation,			// Use eye location, screen center
	WSD_WeaponDirection,		// Use the weapon aim direction
};


UENUM(BlueprintType)
enum EWeaponType
{
	WT_Normal,			
	WT_Holster,			
	WT_Item,			
	WT_Heavy
};


UENUM(BlueprintType)
enum EFiringMode
{
	FM_DEFAULT,
	FM_RELOAD,
	FM_MELEEATTACK
};


USTRUCT(BlueprintType)
struct FHitScanTrace
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};