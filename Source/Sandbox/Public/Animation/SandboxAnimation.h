// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimInstance.h"
#include "CommonAnimTypes.h"
#include "BoneContainer.h"
#include "BoneIndices.h"
#include "Animnodes/AnimNode_BlendListBase.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "SandboxAnimation.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(SandboxAnimLog, All, All)

class UAnimSequence;
struct FAnimTickRecord;
class ASandboxGameModeBase;
class ASandboxPawn;

static inline FVector AnimWorldToLocal(FVector const& WorldVect, FRotator const& SystemRot)
{
	return FRotationMatrix(SystemRot).InverseTransformVector(WorldVect);
}
static inline FVector AnimLocalToWorld(FVector const& LocalVect, FRotator const& SystemRot)
{
	return FRotationMatrix(SystemRot).TransformVector(LocalVect);
}


UENUM(BlueprintType)
enum ESpeedType
{
	EST_Velocity			UMETA(DisplayName = "Velocity"),
	EST_AccelAndMaxSpeed	UMETA(DisplayName = "AccelerationMaxSpeed"),
	EST_RootMotion			UMETA(DisplayName = "RootMotion"),
};

UENUM(BlueprintType)
enum EMoveChannel
{
	EMC_Idle		UMETA(DisplayName = "Idle"),
	EMC_Walk		UMETA(DisplayName = "Walk"),
	EMC_Run			UMETA(DisplayName = "Run"),
	EMC_RoadieRun	UMETA(DisplayName = "Roadie")
};


USTRUCT(BlueprintType)
struct FMovementDefinition
{
	GENERATED_USTRUCT_BODY()

public:

	FMovementDefinition()
		: BaseSpeed(0.f)
		, MoveType(EMoveChannel::EMC_Idle)
	{
		//
	}

	UPROPERTY(EditAnywhere, Category = "MovementDefinition")
	float BaseSpeed;

	UPROPERTY(EditAnywhere, Category = "MovementDefinition")
	TEnumAsByte<EMoveChannel> MoveType;
};


USTRUCT(BlueprintType)
struct FRotTransitionInfo
{
	GENERATED_USTRUCT_BODY()

public:

	/** Constructors */
	FRotTransitionInfo()
		: RotationOffset(0.f)
		, Transition(nullptr)
	{
		//
	}

	UPROPERTY(EditAnywhere, Category = "Settings")
	float RotationOffset;

	UPROPERTY(EditAnywhere, Category = "Settings")
	UAnimSequence* Transition;
};

class USkeletalMeshComponent;

UENUM(BlueprintType)
enum EFootPlacementAxis
{
	EFP_X	UMETA(DisplayName = "Axis X"),
	EFP_Y	UMETA(DisplayName = "Axis Y"),
	EFP_Z	UMETA(DisplayName = "Axis Z")
};

UENUM(BlueprintType)
enum EFootPlacementLeg
{
	EFP_Left	UMETA(DisplayName = "Left Leg"),
	EFP_Right	UMETA(DisplayName = "Right Leg")
};


/**
 * USandboxAnimation
 */
UCLASS()
class SANDBOX_API USandboxAnimation : public UAnimInstance
{
	GENERATED_UCLASS_BODY()
public:

	float IKFootLeftLegAdjust;
	float IKFootRightLegAdjust;
	UPROPERTY(BlueprintReadWrite, Category = "Pawn Direction")
	FVector PawnDirection;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;
public:
	ASandboxPawn* GetSandPawn() const { return SandboxPawn; }
	virtual void NativeBeginPlay() override;
protected:
	ASandboxPawn* SandboxPawn;

};



/**
 * FSandboxMovement
 */
USTRUCT(BlueprintInternalUseOnly)
struct SANDBOX_API FSandboxMovement : public FAnimNode_BlendListBase
{
	GENERATED_USTRUCT_BODY()

public:
	FSandboxMovement();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	TEnumAsByte<ESpeedType>	SpeedType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings", meta = (PinShownByDefault))
	float		Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	bool		bScaleAnimationsPlayRateBySpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	bool		bScaleConstraintsByBaseSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	bool		bShouldHandleTransitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	bool		ForceIdleChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	float		RateSpeedUpperBound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	float		TransitionBlendOutTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	float		IdleBlendOutTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	float		BlendUpTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	float		BlendDownTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	float		BlendDownPerc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	float		MoveCycleFirstStepStartPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	TArray<FMovementDefinition> MovementDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	float		TransWeightResumeTheshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	FName		SynchGroupName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	float		SynchPosOffset;

	UPROPERTY(Transient)
	float	MovementBlendWeight;

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

protected:

	USandboxAnimation* AnimInstance;
	EWorldType::Type MovementWorldType;
	int32	ActiveMovementIndex;
	bool	bPlayingTransitionToIdle;
	float	PrevGroupRelPos;
	float	GroupRelPos;
	float	SynchPctPosition;
	float MovementPlayRate;
	virtual float GetCurrentSpeed(ASandboxPawn* Pawn) const;
	FVector GetMovementNodeSpeedDirection(ASandboxPawn* Pawn) const;
	virtual int32 GetActiveChildIndex() override;
	virtual FString GetNodeName(FNodeDebugData& DebugData) override;
	FAnimTickRecord GetAnimTickRecord(const FAnimationUpdateContext& Context);
};


/**
 * FSandboxFootPlacement
 */
USTRUCT(BlueprintInternalUseOnly)
struct SANDBOX_API FSandboxFootPlacement : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	FSandboxFootPlacement();

	UPROPERTY(EditAnywhere, Category = "IK")
	FBoneReference IKFootBone;

	UPROPERTY(EditAnywhere, Category = "IK")
	TEnumAsByte<EFootPlacementLeg> FootLeg;

	UPROPERTY(EditAnywhere, Category = "IK")
	uint32 bAllowStretch : 1;

	UPROPERTY(EditAnywhere, Category = "IK", meta = (EditCondition = "bAllowStretch", ClampMin = "0.0", UIMin = "0.0"))
	float InitStretchRatio;

	UPROPERTY(EditAnywhere, Category = "IK", meta = (EditCondition = "bAllowStretch", ClampMin = "0.0", UIMin = "0.0"))
	float MaxStretchValue;

	UPROPERTY(EditAnywhere, Category = "IK")
	uint32 bTakeRotationFromEffector : 1;

	UPROPERTY(EditAnywhere, Category = "IK")
	uint32 bKeepEffectorRelRot : 1;

	UPROPERTY(EditAnywhere, Category = "IK")
	bool bAllowBoneTwist;

	UPROPERTY(EditAnywhere, Category = "IK", meta = (EditCondition = "!bAllowBoneTwist"))
	FAxis TwistBoneAxis;

	UPROPERTY(EditAnywhere, Category = "Effector")
	TEnumAsByte<EBoneControlSpace> EffectorBoneSpace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effector", meta = (PinShownByDefault))
	FVector EffectorLocation;

	UPROPERTY(EditAnywhere, Category = "JointTarget")
	TEnumAsByte<EBoneControlSpace> JointTargetBoneSpace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JointTarget", meta = (PinShownByDefault))
	FVector JointTargetLoc;

	UPROPERTY(EditAnywhere, Category = "Effector")
	FBoneSocketTarget EffectorBoneTarget;

	UPROPERTY(EditAnywhere, Category = "JointTarget")
	FBoneSocketTarget JointBoneTarget;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	uint32		bDoFootPlacement : 1;

	UPROPERTY(EditAnywhere, Category = "FootPlacement", meta = (PinShownByDefault))
	float	ResetInterpSpeed;

	UPROPERTY(EditAnywhere, Category = "FootPlacement", meta = (PinShownByDefault))
	FVector	FloorNormal;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	float	FootOffset;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	float	MaxUpAdjustment;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	float	MaxDownAdjustment;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	uint32	bOrientFootToGround : 1;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	TEnumAsByte<EFootPlacementAxis>	FootUpAxis;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	uint32	bInvertFootUpAxis : 1;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	float	MaxFootOrientAdjust;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	float	MaxFootOrientFloorAngle;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	float	FootSize;

	UPROPERTY(EditAnywhere, Category = "FootPlacement")
	FVector  RelativeFootOffset;

	UPROPERTY(EditAnywhere, Category = "FootPlacementPlane")
	uint32	bProjectOntoPlane : 1;

	UPROPERTY(EditAnywhere, Category = "FootPlacementPlane")
	FBoneReference	PlaneBone;

	UPROPERTY(EditAnywhere, Category = "FootPlacementPlane")
	TEnumAsByte<EFootPlacementAxis> PlaneNormal;

	UPROPERTY(EditAnywhere, Category = "FootPlacementPlane")
	uint32	bInvertPlaneNormal : 1;

	UPROPERTY(EditAnywhere, Category = "Performance")
	uint32 bSkipIKWhenNotRendered : 1;

	UPROPERTY(EditAnywhere, Category = "Performance")
	uint32 bEnforceMaxLimitUpdates : 1;

	UPROPERTY(Transient)
	float  LegAdjust;

protected:

	FVector OwnerVelocity;
	FVector LegForwardAdjust;
	FVector TargetEffectorLocation;
	FVector InterpolatedEffectorLocation;
	FRotator TargetEffectorRotation;
	FRotator InterpolatedEffectorRotation;
	FRotator EffectorRotation;
	int32 FootPlacementUpdateTag;
	FCompactPoseBoneIndex CachedUpperLimbIndex;
	FCompactPoseBoneIndex CachedLowerLimbIndex;
#if WITH_EDITOR
	FVector CachedJoints[3];
	FVector CachedJointTargetPos;
	FVector CachedFootPos;
	FVector DebugStartLoc;
	FVector DebugEndLoc;
	FVector DebugIKLoc;
	FVector DebugHitLoc;
#endif // WITH_EDITOR
	USandboxAnimation* AnimInstance;

public:

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

#if WITH_EDITOR
	void DrawDebugBones(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp) const;
#endif

protected:

	virtual void UpdateInternal(const FAnimationUpdateContext& Context);

private:

	void EvaluateTwoBoneIK(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
	static FTransform GetTargetTransform(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases, FBoneSocketTarget& InTarget, EBoneControlSpace Space, const FVector& InOffset);
	void ApplyEffectorRotation(TArray<FBoneTransform>& OutBoneTransforms, FRotator& DesiredEffectorRotation);
	bool LegLineCheck(const AActor* Owner, const FVector& Start, const FVector& End, FVector& CurrentHitLocation, FVector& CurrentHitNormal, const FVector& Extent = FVector::ZeroVector);
	bool DoEnforceMaxLimitUpdates(ASandboxGameModeBase* SandGame);
};

