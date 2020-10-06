#pragma once

#include "Camera/PlayerCameraManager.h"
#include "DisplayDebugHelpers.h"
#include "SandboxCameraTypes.h"
#include "SandboxCameraManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(SandboxCameraLog, All, All)

class UCanvas;


/**
* UGameCameraBase
**/
UCLASS(config = "SandoxCamera")
class SANDBOX_API UGameCameraBase : public UObject
{
	GENERATED_BODY()

public:

	UGameCameraBase();
	UPROPERTY(Transient)
	ASandboxCameraManager* PlayerCamera;
	UPROPERTY(Transient)
	uint8 bResetCameraInterpolation : 1;
	virtual void OnBecomeActive(UGameCameraBase* OldCamera);
	virtual void OnBecomeInActive(UGameCameraBase* NewCamera);
	virtual void UpdateCamera(APawn* P, ASandboxCameraManager* CameraActor, float DeltaTime, FTViewTarget& OutVT);
	virtual void ResetInterpolation();
	virtual void Init();
	virtual void ProcessViewRotation(float DeltaTime, AActor* ViewTarget, FRotator& out_ViewRotation, FRotator& out_DeltaRot);
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos);
};


/**
* UGameFixedCamera
**/
UCLASS(config = "SandoxCamera")
class SANDBOX_API UGameFixedCamera : public UGameCameraBase
{
	GENERATED_BODY()
public:

	UGameFixedCamera();
	virtual void OnBecomeActive(UGameCameraBase* OldCamera) override;
	virtual void UpdateCamera(APawn* P, ASandboxCameraManager* CameraActor, float DeltaTime, FTViewTarget& OutVT) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Fixed Camera")
	float DefaultFOV;
};




/**
* UGameThirdPersonCameraMode
**/
UCLASS(transient, Blueprintable, BlueprintType, meta = (BlueprintThreadSafe), Within = GameThirdPersonCamera)
class SANDBOX_API UGameThirdPersonCameraMode : public UObject
{
	GENERATED_BODY()

public:

	UGameThirdPersonCameraMode();
	UPROPERTY(Transient)
	UGameThirdPersonCamera* ThirdPersonCam;
	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCameraMode")
	float FOVAngle;
	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCameraMode")
	float BlendTime;

	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCameraMode")
	FViewOffsetData ViewOffset;
	uint8 bLockedToViewTarget : 1;
	uint8 bDirectLook : 1;
	uint8 bFollowTarget : 1;
	float FollowingInterpSpeed_Pitch;
	float FollowingInterpSpeed_Yaw;
	float FollowingInterpSpeed_Roll;
	float FollowingCameraVelThreshold;
	uint8 bInterpLocation : 1;
	float OriginLocInterpSpeed;
	uint8 bUsePerAxisOriginLocInterp : 1;
	FVector PerAxisOriginLocInterpSpeed;
	uint8 bInterpRotation : 1;
	float OriginRotInterpSpeed;
	uint8 bRotInterpSpeedConstant : 1;
	FVector StrafeLeftAdjustment;
	FVector StrafeRightAdjustment;
	float StrafeOffsetScalingThreshold;
	float StrafeOffsetInterpSpeedIn;
	float StrafeOffsetInterpSpeedOut;

	UPROPERTY(Transient)
	FVector LastStrafeOffset;
	FVector RunFwdAdjustment;
	FVector RunBackAdjustment;
	float RunOffsetScalingThreshold;
	float RunOffsetInterpSpeedIn;
	float RunOffsetInterpSpeedOut;
	UPROPERTY(Transient)
	FVector LastRunOffset;
	FVector WorstLocOffset;
	uint8 bDoPredictiveAvoidance : 1;
	uint8 bValidateWorstLoc : 1;
	uint8 bSkipCameraCollision : 1;
	FVector TargetRelativeCameraOriginOffset;
	uint8 bSmoothViewOffsetPitchChanges : 1;
	FViewOffsetData ViewOffset_ViewportAdjustments[6];
	uint8 bApplyDeltaViewOffset : 1;
	uint8 bNoFOVPostProcess : 1;
	uint8 bInterpViewOffsetOnlyForCamTransition : 1;
	float ViewOffsetInterp;
	float OffsetAdjustmentInterpSpeed;
	UPROPERTY(Transient)
	TEnumAsByte<ECameraViewportTypes> CurrentViewportType;
	void SetViewOffset(const FViewOffsetData& NewViewOffset);
	virtual FVector GetCameraWorstCaseLoc(APawn* TargetPawn, FTViewTarget& CurrentViewTarget);
	virtual FVector AdjustViewOffset(APawn* P, FVector& Offset);
	virtual FVector GetViewOffset(APawn* ViewedPawn, float DeltaTime, const FVector& ViewOrigin, const FRotator& ViewRotation);
	float GetViewOffsetInterpSpeed(APawn* ViewedPawn, float DeltaTime);
	virtual FRotator GetViewOffsetRotBase(APawn* ViewedPawn, const FTViewTarget& VT);
	virtual void GetBaseViewOffsets(APawn* ViewedPawn, TEnumAsByte<ECameraViewportTypes> ViewportConfig, float DeltaTime, FVector& out_Low, FVector& out_Mid, FVector& out_High);
	virtual bool UseDirectLookMode(APawn* CameraTarget);
	virtual bool LockedToViewTarget(APawn* CameraTarget);
	virtual bool ShouldFollowTarget(APawn* CameraTarget, float& PitchInterpSpeed, float& YawInterpSpeed, float& RollInterpSpeed);
	virtual FVector GetTargetRelativeOriginOffset(APawn* TargetPawn);
	virtual void GetCameraOrigin(APawn* TargetPawn, FVector& OriginLoc, FRotator& OriginRot);
	virtual void InterpolateCameraOrigin(APawn* TargetPawn, float DeltaTime, FVector& out_ActualCameraOrigin, FVector const& IdealCameraOrigin, FRotator& out_ActualCameraOriginRot, FRotator const& IdealCameraOriginRot);
	virtual float GetBlendTime(APawn* Pawn);
	virtual float GetFOVBlendTime(APawn* Pawn);
	virtual FVector InterpolateCameraOriginLoc(APawn* TargetPawn, FRotator const& CameraTargetRot, FVector const& LastLoc, FVector const& IdealLoc, float DeltaTime);
	virtual FRotator InterpolateCameraOriginRot(APawn* TargetPawn, FRotator const& LastRot, FRotator const& IdealRot, float DeltaTime);
	virtual FVector	ApplyViewOffset(APawn* ViewedPawn, const FVector& CameraOrigin, const FVector& ActualViewOffset, const FVector& DeltaViewOffset, const FTViewTarget& OutVT);
	virtual float GetViewPitch(APawn* TargetPawn, FRotator const& ViewRotation) const;
	virtual float GetDesiredFOV(APawn* ViewedPawn);
	virtual void OnBecomeActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* PrevMode);
	virtual void OnBecomeInActiveMode(APawn* TargetPawn, UGameThirdPersonCameraMode* NewMode);
	virtual void Init();
	virtual void ProcessViewRotation(float DeltaTime, AActor* ViewTarget, FRotator& out_ViewRotation, FRotator& out_DeltaRot);
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos);
};



/**
* UGameThirdPersonCameraMode_Default
**/
UCLASS(config = "SandoxCamera")
class SANDBOX_API UGameThirdPersonCameraMode_Default : public UGameThirdPersonCameraMode
{
	GENERATED_BODY()

public:

	UGameThirdPersonCameraMode_Default();
	virtual void Init() override;

protected:
	float WorstLocAimingZOffset;
	UPROPERTY(Transient)
	uint8 bTemporaryOriginRotInterp : 1;
	float TemporaryOriginRotInterpSpeed;

public:
	virtual void GetCameraOrigin(APawn* TargetPawn, FVector& OriginLoc, FRotator& OriginRot) override;
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
};



/**
* UGameThirdPersonCamera
**/
UCLASS(transient, Blueprintable, BlueprintType, meta = (BlueprintThreadSafe), config = "SandoxCamera")
class SANDBOX_API UGameThirdPersonCamera : public UGameCameraBase
{
	GENERATED_BODY()

public:

	UGameThirdPersonCamera();
	UPROPERTY(Transient)
	FVector LastActualOriginOffset;
	float WorstLocBlockedPct;
	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCamera")
	float WorstLocPenetrationExtentScale;

	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCamera")
	float PenetrationBlendOutTime;

	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCamera")
	float PenetrationBlendInTime;

	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCamera")
	float PenetrationExtentScale;

protected:

	float PenetrationBlockedPct;

public:

	UPROPERTY(Transient)
	FVector LastActualCameraOriginLoc;

	UPROPERTY(Transient)
	FRotator LastActualCameraOriginRot;

	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCamera")
	float OriginOffsetInterpSpeed;

	UPROPERTY(Transient)
	FVector LastViewOffset;

	UPROPERTY(Transient)
	float LastCamFOV;

	UPROPERTY(Transient)
	FVector LastIdealCameraOriginLoc;

	UPROPERTY(Transient)
	FRotator LastIdealCameraOriginRot;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "GameThirdPersonCamera")
	FRotator UE3PawnRotation;

protected:

	UPROPERTY(Transient)
	UGameThirdPersonCameraMode* ThirdPersonCamDefault;

public:

	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCamera")
	TSubclassOf<UGameThirdPersonCameraMode> ThirdPersonCamDefaultClass;
	UPROPERTY(Transient)
	UGameThirdPersonCameraMode* CurrentCamMode;

	UPROPERTY(Transient)
	float LastHeightAdjustment;

	UPROPERTY(Transient)
	float LastPitchAdjustment;

	UPROPERTY(Transient)
	float LastYawAdjustment;

	UPROPERTY(Transient)
	float LeftoverPitchAdjustment;


protected:

	UPROPERTY(Transient)
	float TurnCurTime;
	uint8 bDoingACameraTurn : 1;
	float TurnStartAngle;
	float TurnEndAngle;
	float TurnTotalTime;
	float TurnDelay;
	uint8 bTurnAlignTargetWhenFinished : 1;
	UPROPERTY(Transient)
	float LastPostCamTurnYaw;

public:

	UPROPERTY(Transient)
	float DirectLookYaw;

	UPROPERTY(Transient)
	uint8 bDoingDirectLook : 1;

	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCamera")
	float DirectLookInterpSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "GameThirdPersonCamera")
	float WorstLocInterpSpeed;

	UPROPERTY(Transient)
	FVector LastWorstLocationLocal;

	UPROPERTY(Transient)
	FVector LastPreModifierCameraLoc;

	UPROPERTY(Transient)
	FRotator LastPreModifierCameraRot;
	TArray<FPenetrationAvoidanceFeeler> PenetrationAvoidanceFeelers;

	UPROPERTY(Transient)
	FVector LastOffsetAdjustment;

	uint8 bDebugChangedCameraMode : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Camera Debug")
	uint8 bDrawDebug : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Camera Debug")
	uint8 bDrawCameraPawnDebug : 1;

protected:

	virtual void InterpolateCameraOrigin(APawn* TargetPawn, float DeltaTime, FVector& out_ActualCameraOrigin, FVector const& IdealCameraOrigin, FRotator& out_ActualCameraOriginRot, FRotator const& IdealCameraOriginRot);
	void PreventCameraPenetration(APawn* P, class ASandboxCameraManager* CameraActor, const FVector& WorstLocation, FVector& DesiredLocation, float DeltaTime, float& DistBlockedPct, float CameraExtentScale, bool bSingleRayOnly = false);
	virtual void UpdateForMovingBase(UPrimitiveComponent* BaseActor);
	virtual FVector  GetPostInterpCameraOriginLocationOffset(APawn* TargetPawn) const { return FVector::ZeroVector; }
	virtual FRotator GetPostInterpCameraOriginRotationOffset(APawn* TargetPawn) const { return FRotator::ZeroRotator; }
	virtual FMatrix GetWorstCaseLocTransform(APawn* P) const;
	virtual bool ShouldIgnorePenetrationHit(FHitResult Hit, APawn* TargetPawn) const;
	virtual bool ShouldDoPerPolyPenetrationTests(APawn* TargetPawn) const { return false; };
	virtual bool ShouldDoPredictavePenetrationAvoidance(APawn* TargetPawn) const;
	virtual void HandlePawnPenetration(FTViewTarget& OutVT);
	virtual bool HandleCameraSafeZone(FVector& CameraOrigin, FRotator& CameraRotation, float DeltaTime) { return false; }
	virtual UGameThirdPersonCameraMode* CreateCameraMode(TSubclassOf<UGameThirdPersonCameraMode> ModeClass);
	void Reset();
	virtual void PlayerUpdateCamera(APawn* P, class ASandboxCameraManager* CameraActor, float DeltaTime, struct FTViewTarget& OutVT);
	virtual void Init() override;
	virtual float GetDesiredFOV(APawn* ViewedPawn);
	virtual void UpdateCamera(APawn* P, class ASandboxCameraManager* CameraActor, float DeltaTime, FTViewTarget& OutVT) override;

public:

	void BeginTurn(float StartAngle, float EndAngle, float TimeSec, float DelaySec = 0.f, bool bAlignTargetWhenFinished = false);
	virtual void EndTurn();

protected:

	void AdjustTurn(float& AngleOffset);
	virtual UGameThirdPersonCameraMode* FindBestCameraMode(APawn* P);
	void UpdateCameraMode(APawn* P);
	virtual void ProcessViewRotation(float DeltaTime, AActor* ViewTarget, FRotator& out_ViewRotation, FRotator& out_DeltaRot) override;
	virtual void OnBecomeActive(UGameCameraBase* OldCamera) override;
	virtual void ResetInterpolation() override;
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
};



/**
* ASandboxCameraManager
**/
UCLASS(config = "SandoxCamera")
class SANDBOX_API ASandboxCameraManager : public APlayerCameraManager
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(Transient)
	UGameCameraBase* ThirdPersonCam;

	UPROPERTY(EditDefaultsOnly, Category = "GamePlayerCamera")
	TSubclassOf<UGameCameraBase> ThirdPersonCameraClass;

	UPROPERTY(Transient)
	UGameCameraBase* FixedCam;

	UPROPERTY(EditDefaultsOnly, Category = "GamePlayerCamera")
	TSubclassOf<UGameCameraBase> FixedCameraClass;

	UPROPERTY(Transient)
	UGameCameraBase* CurrentCamera;

	UPROPERTY(EditDefaultsOnly, Category = "GamePlayerCamera")
	uint8 bDebugCamera : 1;

	UPROPERTY(EditDefaultsOnly, Category = "GamePlayerCamera")
	uint8 bProcessViewRotation : 1;

public:

	UPROPERTY(Transient)
	uint8 bUseForcedCamFOV : 1;
	UPROPERTY(Transient)
	float ForcedCamFOV;
	UPROPERTY(Transient)
	uint8 bInterpolateCamChanges : 1;
	float SplitScreenShakeScale;

private:

	UPROPERTY(Transient)
	AActor* LastViewTarget;

protected:

	UPROPERTY(Transient)
	uint8 bResetInterp : 1;
	UPROPERTY(Transient)
	AActor* LastTargetBase;
	UPROPERTY(Transient)
	FMatrix LastTargetBaseTM;

public:

	virtual void AddPawnToHiddenActorsArray(APawn* PawnToHide);
	virtual void RemovePawnFromHiddenActorsArray(APawn* PawnToShow);


protected:

	virtual UGameCameraBase* CreateCamera(TSubclassOf<UGameCameraBase> CameraClass);
	virtual void CacheLastTargetBaseInfo(AActor* TargetBase);

public:

	virtual void PostInitializeComponents() override;
	void Reset();

protected:
	virtual UGameCameraBase* FindBestCameraType(AActor* CameraTarget);

public:

	virtual bool ShouldConstrainAspectRatio();

protected:

	float AdjustFOVForViewport(float inHorizFOV, APawn* CameraTargetPawn) const;
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime);

public:
	virtual void ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot) override;
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
};

