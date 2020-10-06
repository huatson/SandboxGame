// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SandboxPawnTypes.h"
#include "SandboxWeaponTypes.h"
#include "SandboxWeapon.h"
#include "SandboxPawn.generated.h"

class USandboxPawnMovement;

UCLASS()
class SANDBOX_API ASandboxPawn : public ACharacter
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Locomotion")
	USandboxPawnMovement* SandboxMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	uint32 bAddLocomotionBindings : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion")
	float BaseTurnRate;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion")
	float BaseLookUpRate;

	uint8 bIsMirrored : 1;

	virtual void PreNetReceive() override;
	virtual void PostNetReceive() override;


	/**
	 * CORE MOVEMENT
	 */
	void MoveForward(float Value);
	void MoveRight(float Value);
	void LookUp(float Value);
	void Turn(float Value);
	void LookUpRate(float Value);
	void TurnRate(float Value);


	// player input settings
	float		aForward;
	float		aTurn;
	float		aStrafe;
	float		aUp;
	float		aLookUp;



	/**
	 * WALKING MOVEMENT
	 */
	 /** Is Pawn Walking? Used by animations. */
	UPROPERTY(Transient, Replicated)
	uint8 bIsWalking : 1;

	/** update Walking state */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerSetWalking(bool bNewWalking);

	/** [server + local] change Walking state */
	void SetWalking(bool bNewWalking);

	/** player pressed Walking action */
	void OnStartWalking();

	/** player released Walking action */
	void OnStopWalking();


	/**
	* ROADIERUN
	*/

	/** This pawn is capable of ... */
	uint8		bCanRoadieRun : 1;
	uint8		bCanBeForcedToRoadieRun : 1; 
	uint8       bCanRoadieRunWhileTargeting : 1;
	uint8		bCanBlindFire : 1;
	
	UPROPERTY(Transient, Replicated)
	uint8 bIsRoadieRunning : 1;

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerSetRoadieRun(bool bNewRoadieRun);

	/** [server + local] change RoadieRun state */
	void SetRoadieRun(bool bNewRoadieRun);

	/** Amount of time before starting a roadie run */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion")
	float RoadieRunTimer;

	FTimerHandle TimerHandle_TryToRoadieRun;

	/**
	* Tries to perform the Roadie Run special move.
	*/
	void TryToRoadieRun();

	/** player pressed RoadieRun action */
	void OnStartRoadieRun();

	/** player released RoadieRun action */
	void OnStopRoadieRun();

	float RoadieTurnRateValue;
	


	/**
	* TARGETING/RELOADING/FIRE
	*/

	/** Is Pawn targeting? Used by animations. */
	UPROPERTY(Transient, Replicated)
	uint8 bIsTargeting : 1;

	/** update targeting state */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerSetTargeting(bool bNewTargeting);

	/** [server + local] change targeting state */
	void SetTargeting(bool bNewTargeting);

	/** Returns TRUE if Pawn is currently reloading his weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsReloadingWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FVector GetAimOffsetOrigin() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FRotator GetAimOffsets() const;

	UFUNCTION(BlueprintCallable, Category = "Player")
	bool IsInAimingPose();

	/** get targeting state */
	UFUNCTION(BlueprintCallable, Category = "Player")
	bool IsTargeting() const;

	/** player pressed targeting action */
	void OnStartTargeting();

	/** player released targeting action */
	void OnStopTargeting();

	/** player pressed start fire action */
	void StartFire();

	/** player released start fire action */
	void StopFire();


	/**
	 * POI
	 */
	
	 /** Is Pawn POI? Used by animations. */
	UPROPERTY(Transient, Replicated)
	uint8 bIsPOI : 1;
	void SetPOI(bool bNewPOI);
	void StartPOI();
	void StopPOI();


protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
public:

	/*	Floor Conform and Mesh Translation	*/
	void TickSpecial(float DeltaTime);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	/**
	* CAMERA SETTINGS
	*/
	uint8 bIsHiddenByCamera : 1;
	float	CameraBoneMotionScale;
	UPROPERTY(EditDefaultsOnly, Category = "CameraSettings")
	float RoadieRunShakyCamDampeningFactor;
	UPROPERTY(EditDefaultsOnly, Category = "CameraSettings")
	FVector ExtraThirdPersonCameraOffset;


	/**
	* HEALTH
	*/
	float Health;
	float HealthMax;
	virtual bool IsAliveAndWell() const;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;



	/**
	* COVER SETTINGS
	*/
	UPROPERTY(Transient, Replicated)
	TEnumAsByte<ECoverType> CoverType;
	float CoverMovementPct[3];
	UPROPERTY(Transient, Replicated)
	TEnumAsByte<ECoverAction>	CoverAction;
	TEnumAsByte<ECoverAction>	PreviousCoverAction;
	TEnumAsByte<ECoverAction>	SavedCoverAction;
	TEnumAsByte<ECoverDirection>  CoverDirection;


	/**
	* CRINGE SETTINGS
	*/
	float LastShotAtTime;
	int32 StopCringeCount;
	float LastCringeTime;
	int32 CringeCount;
	TArray<FCringeInfo>	NearHitHistory;
	TArray<float>		CringeHistory;
	FTimerHandle TH_PlayNearMissCringe;
	virtual bool PlayNearMissCringe();
	virtual bool TryPlayNearMissCringe();


	/**
	* FIRE SETTINGS
	*/

	void SetCurrentWeapon(ASandboxWeapon* NewWeapon, ASandboxWeapon* LastWeapon = nullptr);

	UPROPERTY(Replicated)
	TEnumAsByte<EFiringMode> FiringMode;
	UPROPERTY(Transient)
	float LastWeaponStartFireTime;
	UPROPERTY(Transient)
	float LastWeaponEndFireTime;
	UPROPERTY(Transient)
	float	ForcedAimTime;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float	AimTimeAfterFiring;

	UPROPERTY(Transient)
	uint8 bPendingFire : 1;

	UPROPERTY(Transient)
	uint8 bPendingAltFire : 1;

	UPROPERTY(Transient)
	uint8 bPendingMeleeAttack : 1;

	UPROPERTY(Transient, Replicated)
	uint8 bIsInCombat : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8 bCombatSpeedModifier : 1;




	/**
	* SLOT ATTACHMENTS
	*/

	UPROPERTY(EditDefaultsOnly, Category = "Slot Attachments")
	FName PelvisBoneName;

	UPROPERTY(EditDefaultsOnly, Category = "Slot Attachments")
	FName	NeckBoneName;

	UPROPERTY(EditDefaultsOnly, Category = "Slot Attachments")
	FName	MeleeDamageBoneName;

	UPROPERTY(EditDefaultsOnly, Category = "Slot Attachments")
	FVector	ViewLocationOffset;





	/**
	* WEAPON SETTINGS
	*/

	UPROPERTY(Transient, Replicated)
	TArray<ASandboxWeapon*> WeaponInventory;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<ASandboxWeapon> StarterWeaponClass;

	/** current weapon rep handler */
	UFUNCTION()
	void OnRep_CurrentWeapon(ASandboxWeapon* LastWeapon);

	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentWeapon)
	ASandboxWeapon* CurrentWeapon;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FORCEINLINE ASandboxWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponSocket;



	/**
	* MESH TRANSLATION
	*/

	uint32		bDoMeshLocSmootherOffset : 1;
	uint32		bCanDoFloorConform : 1;
	UPROPERTY(EditDefaultsOnly, Category = "MeshFloorConform")
	uint32		bDoFloorConform : 1;
	UPROPERTY(Transient)
	int32		FloorConformUpdateTag;
	UPROPERTY(Transient)
	FVector		FloorConformAverageFloorNormal;
	UPROPERTY(Transient)
	FVector		FloorConformTranslation;
	UPROPERTY(Transient)
	float		FloorConformZTranslation;
	float		FloorConformMaxSlopeAngle;
	FRotator	FloorConformLastPawnRotation;
	UPROPERTY(EditDefaultsOnly, Category = "MeshFloorConform")
	uint32				bDoFullMeshFloorConform : 1;
	FTAlphaBlend		MeshFloorConformMovementAlpha;
	FTAlphaBlend		MeshFloorConformToggleAlpha;
	FVector				MeshFloorConformNormal;
	FVector				MeshFloorConformBackupNormal;
	float				MeshFloorConformTranslation;
	float				MeshFloorConformSlopeAngle;
	FRotator			MeshFloorConformRotation;
	FTAlphaBlend	IKFloorConformMovementAlpha;
	UPROPERTY(Transient)
	FVector			IKFloorConformNormal;
	UPROPERTY(Transient)
	FVector			IKFloorConformBackupNormal;
	UPROPERTY(Transient)
	float			IKFloorConformTranslation;
	UPROPERTY(Transient)
	float			IKFloorConformSlopeAngle;
	UPROPERTY(Transient)
	FRotator		IKFloorConformRotation;
	uint32			bDoIKFloorConformTorsoBending : 1;
	UPROPERTY(Transient)
	FQuat			IKFloorConformTorsoQuatWorld;
	UPROPERTY(Transient)
	FQuat			IKFloorConformTorsoQuatLocal;
	FVector			IKFloorConformCOMTranslation;
	uint32			bDoIKFloorConformCOM : 1;
	float			IKFloorConformCOMTimeAccumulator;
	UPROPERTY(Transient)
	FVector			IKFloorConformCOMTarget;
	UPROPERTY(EditDefaultsOnly, Category = "MeshFloorConform")
	FVector2D		IKFloorConformCOMLimits;
	UPROPERTY(EditDefaultsOnly, Category = "MeshFloorConform")
	FVector			IKFloorConformCOMSpeed;
	UPROPERTY(EditDefaultsOnly, Category = "MeshFloorConform")
	FVector			IKFloorConformCOMDamping;
	UPROPERTY(Transient)
	FVector			IKFloorConformCOMVelocity;
	UPROPERTY(EditDefaultsOnly, Category = "MeshFloorConform")
	float			IKFloorConformCOMWeight;
	UPROPERTY(EditDefaultsOnly, Category = "MeshFloorConform")
	float			LegAdjustInterpSpeedStopped;
	UPROPERTY(EditDefaultsOnly, Category = "MeshFloorConform")
	float			LegAdjustInterpSpeedMoving;
	UPROPERTY(Transient)
	float			LegAdjust;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FloorConform")
	FVector			FootPlacementLegAdjust;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FloorConform")
	float			IKFloorConformAlpha;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FloorConform")
	FRotator		IKFloorConformBoneRot;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FloorConform")
	FVector			IKFloorConformBoneLoc;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FloorConform")
	FRotator		LegFloorConformBoneRot;
	uint32			bDoFootPlacement : 1;
	FTAlphaBlend	FootPlacementAlpha;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKFoot")
	float			IKFootLeftAlpha;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKFoot")
	float			IKFootRightAlpha;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKFoot")
	FVector			IKFootLeftFloorNormal;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKFoot")
	FVector			IKFootRightFloorNormal;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKFoot")
	float			IKFootResetInterpSpeed;
	UPROPERTY(Transient, EditDefaultsOnly, Category = "IKHands")
	uint8			bDoIKBoneCtrlRightHand : 1;
	UPROPERTY(Transient, EditDefaultsOnly, Category = "IKHands")
	uint8			bDoIKBoneCtrlLeftHand : 1;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKHands")
	float			IKBoneCtrlRightHandAlpha;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKHands")
	float			IKBoneCtrlLeftHandAlpha;
	uint32		bDisableMeshTranslationChanges : 1;
	uint32		bUseFloorImpactNormal : 1;
	UPROPERTY(EditDefaultsOnly, Category = "FloorConform")
	uint32	bCanDoStepsSmoothing : 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FloorConform")
	uint8 bDebugFloorConform : 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Transient, Category = "Weapon")
	FName	LeftWeaponBoneName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Transient, Category = "Weapon")
	FName	RightWeaponBoneName;
	UPROPERTY(EditDefaultsOnly, Category = "FloorConform")
	float              TradeAnimDistance;
	UPROPERTY(Transient)
	FVector		MTO_MeshLocSmootherOffset;
	UPROPERTY(Transient)
	FVector		MTO_SpecialMoveOffset;
	UPROPERTY(Transient)
	FVector		MTO_SpecialMoveInterp;
	UPROPERTY(Transient)
	float		MTO_SpecialMoveSpeed;
	UPROPERTY(Transient)
	float MeshYawOffset;
	float MeshRotationSpeed;
	UPROPERTY(EditDefaultsOnly, Category = "FloorConform")
	float AlphaInterpSpeed;
	uint8			bLockIKStatus : 1;
	UPROPERTY(EditDefaultsOnly, Category = "FloorConform")
	float TurningRadius;
	UPROPERTY(Transient)
	float EffectiveTurningRadius;
	UPROPERTY(EditDefaultsOnly, Category = "FloorConform")
	uint32 bAllowAccelSmoothing : 1;
	UPROPERTY(Transient)
	FVector OldAcceleration;
	UPROPERTY(EditDefaultsOnly, Category = "FloorConform")
	uint32 bAllowTurnSmoothing : 1;
	UPROPERTY(EditDefaultsOnly, Category = "FloorConform")
	uint32			bDoIKFloorConformSkelControl : 1;
	UPROPERTY(EditDefaultsOnly, Category = "FloorConform")
	uint32			bDoLegFloorConformSkelControl : 1;


	void PerformStepsSmoothing(const FVector& OldLocation, float DeltaTime);
	bool GetFloorConformTranslationAndAngle(FVector &InFloorNormal, float &OutTranslation, float &OutSlopeAngle, FVector &OutFloorNormal2D);
	void UpdateNormalInterpolation(FTAlphaBlend& AlphaInterpolator, FVector& CurrentNormal, const FVector& TargetNormal, FVector& BackupNormal, float DeltaTime, float InterpolationSpeed);
	FVector GetFloorConformNormal();
	void UpdateFloorConform(float DeltaTime);
	bool UpdateMeshTranslationOffset(FVector NewOffset, bool bForce = false);
	void MeshLocSmootherOffset(float DeltaTime);


	/**
	* EVADING MOVEMENT
	*/
	virtual bool IsEvading() const;
};
