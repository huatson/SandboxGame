#pragma once

#include "SandboxCameraManager.h"
#include "Camera/CameraModifier.h"
#include "SandboxPawnTypes.h"
#include "SandboxGameCamera.generated.h"



class UAnimSequence;
class USkeletalMeshComponent;
class ASandboxPawn;
class ASandboxController;
class ASandboxWeapon;



/**
 * USandboxGameplayCameraMode
 */
UCLASS(config = "SandoxGameCamera")
class SANDBOX_API USandboxGameplayCameraMode : public UGameThirdPersonCameraMode
{
	GENERATED_BODY()

public:

	USandboxGameplayCameraMode();

	UPROPERTY(Transient)
	USandboxGameplayCamera* GameplayCam;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "SandboxGameplayCameraMode")
	float SpectatorCameraRotInterpSpeed;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SandboxGameplayCameraMode")
	float FOVAngle_ViewportAdjustments[6];
	float TemporaryPivotInterpSpeed;
	uint8 bTemporaryPivotInterp : 1;
	float LastTemporaryPivotInterpAlpha;
	FRotator TemporaryPivotInterpStartingRotation;
	virtual FVector InterpolateCameraOriginLoc(APawn* TargetPawn, FRotator const& CameraTargetRot, FVector const& LastLoc, FVector const& IdealLoc, float DeltaTime) override;
	virtual FRotator InterpolateCameraOriginRot(APawn* TargetPawn, FRotator const& LastRot, FRotator const& IdealRot, float DeltaTime) override;
	float GetViewPitch(APawn * TargetPawn, FRotator const & ViewRotation) const override;
public:
	virtual void GetCameraOrigin(APawn * TargetPawn, FVector & OriginLoc, FRotator & OriginRot) override;
	virtual void GetBaseViewOffsets(APawn* ViewedPawn, TEnumAsByte<ECameraViewportTypes> ViewportConfig, float DeltaTime, FVector& out_Low, FVector& out_Mid, FVector& out_High) override;
	virtual FVector AdjustViewOffset(APawn* P, FVector& Offset) override;
	virtual float GetDesiredFOV(APawn* ViewedPawn) override;
	virtual void WeaponChanged(ASandboxController* C, ASandboxWeapon* OldWeapon, ASandboxWeapon* NewWeapon);
	virtual FVector GetCameraWorstCaseLoc(APawn* TargetPawn, FTViewTarget& CurrentViewTarget) override;
	void StartTransitionPivotInterp();
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos);
};




/**
 * USandboxGameplayCamDefault
 */
UCLASS(config = "SandoxGameCamera")
class SANDBOX_API USandboxGameplayCamDefault : public USandboxGameplayCameraMode
{
	GENERATED_BODY()

public:

	USandboxGameplayCamDefault();

protected:

	UPROPERTY(EditDefaultsOnly, Category = "GameplayCameraDefault")
	FVector		ViewOffsetAdjustment_Evade;

	UPROPERTY(EditDefaultsOnly, Category = "GameplayCameraDefault")
	float		WorstLocAimingZOffset;

	UPROPERTY(Transient)
	uint8	bTemporaryOriginRotInterp : 1;

	UPROPERTY(EditDefaultsOnly, Category = "GameplayCameraDefault")
	float	TemporaryOriginRotInterpSpeed;

	virtual FVector AdjustViewOffset(APawn* P, FVector& Offset) override;
	virtual FVector GetCameraWorstCaseLoc(APawn* TargetPawn, FTViewTarget& CurrentViewTarget) override;
	virtual void OnBecomeActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* PrevMode) override;
};





/**
 * USandboxGameplayRoadieRun
 */
UCLASS(config = "SandoxGameCamera")
class SANDBOX_API USandboxGameplayRoadieRun : public USandboxGameplayCamDefault
{
	GENERATED_BODY()

public:

	USandboxGameplayRoadieRun();
	virtual void OnBecomeActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* PrevMode) override;

protected:
	float BoomerBlendTime;
};




/**
 * USandboxGameplayTargeting
 */
UCLASS(config = "SandoxGameCamera")
class SANDBOX_API USandboxGameplayTargeting : public USandboxGameplayCamDefault
{
	GENERATED_BODY()

public:

	USandboxGameplayTargeting();

protected:

	FVector ViewOffsetAdjustment_Crouch;
	FVector ViewOffsetAdjustment_Kidnapper;

public:

	virtual float GetDesiredFOV(APawn* ViewedPawn) override;
	virtual void OnBecomeActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* PrevMode) override;
	virtual void OnBecomeInActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* NewMode) override;
	virtual void WeaponChanged(ASandboxController* C, ASandboxWeapon* OldWeapon, ASandboxWeapon* NewWeapon) override;
	virtual FVector AdjustViewOffset(APawn* P, FVector& Offset) override;


};




/**
 * UGameplayCameraMode
 */
UCLASS(config = "SandoxGameCamera")
class SANDBOX_API USandboxGameplayCamera : public UGameThirdPersonCamera
{
	GENERATED_BODY()

public:

	USandboxGameplayCamera();

	UPROPERTY(Transient)
	UGameThirdPersonCameraMode* SandCamRoadie;

	UPROPERTY(Transient)
	UGameThirdPersonCameraMode* SandCamTargeting;

	UPROPERTY(Transient)
	uint8 bConversationMode : 1;

	UPROPERTY(Transient)
	ASandboxPawn* LastPawn;

	UPROPERTY(Transient)
	float SnapToLookatPointTimeRemaining;

	UPROPERTY(Transient)
	FVector SnapToLookatPointWorld;
	FRotator LastSnapToLookatDeltaRot;
	ECoverAction SnapToLookatCachedCoverAction;
	ECoverDirection SnapToLookatCachedCoverDir;
	FVector HideViewTargetExtent;
	FVector HideViewTargetExtent_FlickerBuffer;

	UAnimMontage* CustomAnimName;

	virtual bool ShouldDoPerPolyPenetrationTests(APawn* TargetPawn) const override;
	virtual bool ShouldDoPredictavePenetrationAvoidance(APawn* TargetPawn) const override;
	FVector GetHideViewTargetExtent() const;
	virtual void HandlePawnPenetration(FTViewTarget& OutVT) override;
	virtual FVector GetPostInterpCameraOriginLocationOffset(APawn* TargetPawn) const override;
	virtual FMatrix GetWorstCaseLocTransform(APawn* P) const override;
	virtual void UpdateForMovingBase(UPrimitiveComponent* BaseActor) override;
	virtual bool ShouldIgnorePenetrationHit(FHitResult Hit, APawn* TargetPawn) const override;
	FRotator GetOriginRotThatPointsCameraAtPoint(FVector PivotPos, FRotator PivotRot, FVector CameraPos, FRotator CameraRot, FVector LookatWorld);
	virtual void Init() override;
	virtual UGameThirdPersonCameraMode* CreateCameraMode(TSubclassOf<UGameThirdPersonCameraMode> ModeClass) override;
	virtual UGameThirdPersonCameraMode* FindBestCameraMode(APawn* P) override;
	virtual void OnBecomeInActive(UGameCameraBase* NewCamera) override;
	void ForceNextFrameLookatPoint(FVector LookatPointWorld, float Duration);
	virtual void UpdateCamera(APawn* P, class ASandboxCameraManager* CameraActor, float DeltaTime, FTViewTarget& OutVT) override;

public:
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos);
};


/**
 * USandboxFixedCamera
 */
UCLASS(config = "SandoxGameCamera")
class SANDBOX_API USandboxFixedCamera : public UGameFixedCamera
{
	GENERATED_BODY()

public:

	USandboxFixedCamera();

};



/**
 * ASandboxGameCamera
 */
UCLASS(config = "SandoxGameCamera")
class SANDBOX_API USandboxCameraBone : public UCameraModifier
{
	GENERATED_UCLASS_BODY()

public:

	USandboxCameraBone();

	UPROPERTY(EditDefaultsOnly, Category = "CameraBone")
	FName CameraBoneName;
	UPROPERTY(EditDefaultsOnly, Category = "CameraBone")
	FName CameraBoneRootName;

	ASandboxPawn* Pawn;
	uint8	bInitialized : 1;
	USkeletalMeshComponent* CameraBoneMesh;

	UPROPERTY(EditDefaultsOnly, Category = "CameraBone")
	float InCombatAlphaBonus;

	UPROPERTY(EditDefaultsOnly, Category = "CameraBone")
	float AlphaWhenIdle;

	UPROPERTY(EditDefaultsOnly, Category = "CameraBone")
	FVector MaxTranslation;

	UPROPERTY(EditDefaultsOnly, Category = "CameraBone")
	uint8	 bDisableAmbientCameraMotion : 1;

	UPROPERTY(EditDefaultsOnly, Category = "CameraBone")
	uint8	bShakyCamDampening : 1;

	UPROPERTY(EditDefaultsOnly, Category = "CameraBone")
	float	ShakyCamDampeningFactor;

private:

	UPROPERTY(EditDefaultsOnly, Category = "CameraBones")
	float AlphaInterpSpeed;

protected:

	UPROPERTY(Transient)
	FVector LastDeltaPos;

	UPROPERTY(Transient)
	FRotator LastDeltaRot;

	UPROPERTY(EditDefaultsOnly, Category = "CameraBones")
	TEnumAsByte<ECameraAnimOption> CameraAnimOption;

	FRotator CalcDeltaRot(FRotator CameraRootRot, FRotator CameraBoneRot, FRotator PawnRot);
	FRotator FixCamBone(FRotator A) const;
	virtual float GetTargetAlpha() override;
	bool Initialize();

public:
	virtual bool ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV) override;
	virtual void UpdateAlpha(float DeltaTime) override;
};



/**
 * ASandboxGameCamera
 */
UCLASS(config = "SandoxGameCamera")
class SANDBOX_API ASandboxGameCamera : public ASandboxCameraManager
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(Transient)
	USandboxGameplayCamera* GameplayCam;

private:

	UPROPERTY(Transient)
	UCameraModifier* SandboxCameraBone;

public:

	virtual void AddPawnToHiddenActorsArray(APawn* PawnToHide) override;
	virtual void RemovePawnFromHiddenActorsArray(APawn* PawnToShow) override;
	virtual void PostInitializeComponents() override;
	virtual UGameCameraBase* FindBestCameraType(AActor* CameraTarget) override;
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
	void ApplyTransientCameraAnimScale(float Scale);
};
