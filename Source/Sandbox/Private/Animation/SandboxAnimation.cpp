// Fill out your copyright notice in the Description page of Project Settings.


#include "SandboxAnimation.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_StateMachine.h"
#include "Animation/AnimNode_AssetPlayerBase.h"
#include "Animation/BlendSpaceBase.h"
#include "AnimationRuntime.h"
#include "TwoBoneIK.h"
#include "AnimationCoreLibrary.h"
#include "BoneControllers/AnimNode_TwoBoneIK.h"
#include "SandboxGameModeBase.h"
#include "SandboxPawn.h"
#include "SandboxPawnMovement.h"


DEFINE_LOG_CATEGORY(SandboxAnimLog)


/**
 * USandboxAnimation
 */
USandboxAnimation::USandboxAnimation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USandboxAnimation::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void USandboxAnimation::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void USandboxAnimation::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
}

void USandboxAnimation::NativeBeginPlay()
{
	if (AnimInstanceProxy && TryGetPawnOwner())
	{
		SandboxPawn = Cast<ASandboxPawn>(TryGetPawnOwner());
		if (SandboxPawn)
		{
			UE_LOG(SandboxAnimLog, Warning, TEXT("Using pawn: %s"), *GetNameSafe(SandboxPawn))
		}
		else
		{
			UE_LOG(SandboxAnimLog, Warning, TEXT("Using pawn: %s"), *GetNameSafe(TryGetPawnOwner()))
		}
		
	}
}





/**
 * FSandboxMovement
 */
FSandboxMovement::FSandboxMovement()
{
	Speed = 0.0f;
	ForceIdleChannel = false;
	SynchPosOffset = 0.0f;
	MovementBlendWeight = 0.0f;
	
	//	bool options
	bScaleAnimationsPlayRateBySpeed = true;
	bScaleConstraintsByBaseSpeed = true;
	bShouldHandleTransitions = true;

	//	float 
	RateSpeedUpperBound = 10.f;
	IdleBlendOutTime = 0.4f;
	BlendUpTime = 0.4f;
	BlendDownTime = 0.4f;
	BlendDownPerc = 0.33f;
	TransitionBlendOutTime = 0.4f;
	MoveCycleFirstStepStartPosition = 0.3f;
	TransWeightResumeTheshold = 0.5f;

	//	Custom Node
	SynchGroupName = TEXT("RunWalk");
	SpeedType = ESpeedType::EST_Velocity;

	// Default movement definitions
	MovementDefinitions.Empty();
	MovementDefinitions.AddUninitialized(4);
	MovementDefinitions[0].BaseSpeed = 0.0f;
	MovementDefinitions[0].MoveType = EMoveChannel::EMC_Idle;
	MovementDefinitions[1].BaseSpeed = 100.0f;
	MovementDefinitions[1].MoveType = EMoveChannel::EMC_Walk;
	MovementDefinitions[2].BaseSpeed = 300.0f;
	MovementDefinitions[2].MoveType = EMoveChannel::EMC_Run;
	MovementDefinitions[3].BaseSpeed = 450.0f;
	MovementDefinitions[3].MoveType = EMoveChannel::EMC_RoadieRun;
}

void FSandboxMovement::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_BlendListBase::Initialize_AnyThread(Context);
	checkSlow(MovementDefinitions.Num() == 4);

	AnimInstance = Cast<USandboxAnimation>(Context.AnimInstanceProxy->GetAnimInstanceObject());
	if (AnimInstance)
	{
		UE_LOG(SandboxAnimLog, Warning, TEXT("FSandboxMovement::Initialize %s"), *GetNameSafe(AnimInstance))
	}
}

void FSandboxMovement::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	FAnimNode_BlendListBase::CacheBones_AnyThread(Context);
}

void FSandboxMovement::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	FAnimNode_BlendListBase::Update_AnyThread(Context);
	checkSlow(MovementDefinitions.Num() == 4);
	MovementBlendWeight = Context.GetFinalBlendWeight();

	bool bSetChannelBySpeed = true;
	ASandboxPawn* SandboxPawn = AnimInstance->GetSandPawn();
	const float CurrentSpeed = GetCurrentSpeed(SandboxPawn);
	bool bIsInCover = SandboxPawn && (SandboxPawn->CoverType != ECoverType::CT_None);
	bool bForceIdle = false;
	FAnimTickRecord CurrentLeader = GetAnimTickRecord(Context);
	if (!bForceIdle && ForceIdleChannel)
	{
		bForceIdle = true;
	}
	if (bSetChannelBySpeed || bForceIdle)
	{
		int32 TargetChannel = ActiveMovementIndex;
		if (CurrentSpeed <= KINDA_SMALL_NUMBER
			|| (AnimInstance->GetSandPawn() && !SandboxPawn->IsAliveAndWell())
			|| bForceIdle)
		{
			TargetChannel = EMoveChannel::EMC_Idle;
		}
		else
		{
			const float GroundSpeedScale = bScaleConstraintsByBaseSpeed && SandboxPawn
				? FMath::Clamp<float>(SandboxPawn->GetCharacterMovement()->GetMaxSpeed() / 300.f, 0.5f, 2.f) : 1.0f;

			TargetChannel = MovementDefinitions.Num() - 1;
			for (int32 i = MovementDefinitions.Num() - 1; i > EMoveChannel::EMC_Walk; i--)
			{
				const float PrevSpeed = MovementDefinitions[i - 1].BaseSpeed * GroundSpeedScale;
				const float Threshold = PrevSpeed + (MovementDefinitions[i].BaseSpeed * GroundSpeedScale - PrevSpeed) * BlendDownPerc;

				if (CurrentSpeed <= Threshold)
				{
					TargetChannel = i - 1;
				}
			}

			if (SandboxPawn)
			{
				// If stopping power is applied and we're roadie running, then force us in that stance.
				if (SandboxPawn->bIsRoadieRunning && CurrentSpeed > 10.f)
				{
					TargetChannel = EMoveChannel::EMC_RoadieRun;
				}
			}
		}
		if (TargetChannel == EMoveChannel::EMC_RoadieRun && (SandboxPawn && !SandboxPawn->bIsRoadieRunning))
		{
			if ((!SandboxPawn->bCanRoadieRun && !SandboxPawn->bCanBeForcedToRoadieRun) || (SandboxPawn->GetCurrentWeapon() && !SandboxPawn->GetCurrentWeapon()->bAllowsRoadieRunning))
			{
				TargetChannel--;
			}
		}

		if (!bIsInCover && TargetChannel == EMoveChannel::EMC_RoadieRun && (SandboxPawn && SandboxPawn->bIsRoadieRunning))
		{
			const FVector DirectionNormal = GetMovementNodeSpeedDirection(SandboxPawn);
			if ((DirectionNormal | SandboxPawn->GetActorRotation().Vector()) < 0.3f)
			{
				TargetChannel--;
			}
		}

		if (TargetChannel >= MovementDefinitions.Num())
		{
			TargetChannel = MovementDefinitions.Num() - 1;
		}
		if (TargetChannel != ActiveMovementIndex)
		{
			if (ActiveMovementIndex < 0 || BlendPose.IsValidIndex(ActiveMovementIndex))
			{
				float DesiredBlendTime = 0.f;

				// Blending to Idle
				if (TargetChannel < EMoveChannel::EMC_Walk)
				{
					DesiredBlendTime = TransitionBlendOutTime;
				}
				// Blending from idle to movement
				else if (ActiveMovementIndex < EMoveChannel::EMC_Walk)
				{
					DesiredBlendTime = IdleBlendOutTime;
				}
				else if (TargetChannel < ActiveMovementIndex)
				{
					DesiredBlendTime = BlendDownTime;
				}
				else
				{
					DesiredBlendTime = BlendUpTime;
				}

				// Blending from Idle to Movement. See if we need to reset starting position.
				if (ActiveMovementIndex < EMoveChannel::EMC_Walk && TargetChannel >= EMoveChannel::EMC_Walk)
				{
					if (SynchGroupName != EName::NAME_None)
					{
						// If no movement animation is being played, then start from right spot
						if (CurrentLeader.bCanUseMarkerSync && CurrentLeader.EffectiveBlendWeight <= ZERO_ANIMWEIGHT_THRESH)
						{
							
						}
					}
				}
				//SetActiveChild(TargetChannel, bJustBecameRelevant ? 0.f : BlendTime);
				ActiveMovementIndex = TargetChannel;
				BlendTime[ActiveMovementIndex] = DesiredBlendTime;
			}
		}

	}
	if (bScaleAnimationsPlayRateBySpeed && CurrentLeader.SourceAsset)
	{
		if (ActiveMovementIndex >= EMoveChannel::EMC_Walk && ActiveMovementIndex < MovementDefinitions.Num())
		{
			const float Rate = FMath::Min<float>(CurrentSpeed / MovementDefinitions[ActiveMovementIndex].BaseSpeed, RateSpeedUpperBound);
			if (UAnimSequence* CurrentAnimation = Cast<UAnimSequence>(CurrentLeader.SourceAsset))
			{
				CurrentAnimation->RateScale = Rate;
			}
			else if (UBlendSpaceBase* BlendSpace = Cast<UBlendSpaceBase>(CurrentLeader.SourceAsset))
			{
				const TArray<FBlendSample>& BlendSamples = BlendSpace->GetBlendSamples();
				for (FBlendSample CurentBlend : BlendSamples)
				{
					if (CurentBlend.Animation)
					{
						CurentBlend.Animation->RateScale = Rate;
					}
				}
			}
		}
	}
}

void FSandboxMovement::Evaluate_AnyThread(FPoseContext& Output)
{
	FAnimNode_BlendListBase::Evaluate_AnyThread(Output);
}

void FSandboxMovement::GatherDebugData(FNodeDebugData& DebugData)
{
	FAnimNode_BlendListBase::GatherDebugData(DebugData);
}

float FSandboxMovement::GetCurrentSpeed(ASandboxPawn* Pawn) const
{
	if (!Pawn)
	{
		return Speed;
	}

	if (SpeedType == ESpeedType::EST_Velocity)
	{
		return Pawn->GetVelocity().Size2D();
	}
	else if (SpeedType == ESpeedType::EST_RootMotion && Pawn->GetMesh())
	{
		return Pawn->GetMesh()->ConsumeRootMotion().GetRootMotionTransform().GetLocation().Size2D();
	}
	else if (SpeedType == ESpeedType::EST_AccelAndMaxSpeed && !Pawn->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero())
	{
		return Pawn->GetCharacterMovement()->GetMaxSpeed() * Pawn->SandboxMovementComponent->GetGlobalSpeedModifier();
	}

	return 0.f;
}

FVector FSandboxMovement::GetMovementNodeSpeedDirection(ASandboxPawn* Pawn) const
{
	if (Pawn)
	{
		if (SpeedType == EST_Velocity)
		{
			return Pawn->GetVelocity().GetSafeNormal2D();
		}
		else if (SpeedType == EST_RootMotion && Pawn->GetMesh())
		{
			return Pawn->GetMesh()->ConsumeRootMotion().GetRootMotionTransform().GetLocation().GetSafeNormal2D();
		}
		else if (SpeedType == EST_AccelAndMaxSpeed && !Pawn->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero())
		{
			return Pawn->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
		}
	}

	return FVector::ForwardVector;
}

int32 FSandboxMovement::GetActiveChildIndex()
{
	const int32 NumPoses = BlendPose.Num();
	return FMath::Clamp<int32>(ActiveMovementIndex, 0, NumPoses - 1);
}

FString FSandboxMovement::GetNodeName(FNodeDebugData& DebugData)
{
	return DebugData.GetNodeName(this);
}

FAnimTickRecord FSandboxMovement::GetAnimTickRecord(const FAnimationUpdateContext& Context)
{
	const int32 SyncGroupIndex = Context.AnimInstanceProxy->GetAnimClassInterface()->GetSyncGroupIndex(SynchGroupName);
	const TArray<FAnimGroupInstance>& SyncGroups = Context.AnimInstanceProxy->GetSyncGroupRead();
	if (SyncGroups.IsValidIndex(SyncGroupIndex))
	{
		const FAnimGroupInstance& SyncGroupInstance = SyncGroups[SyncGroupIndex];
		if (SyncGroupInstance.bCanUseMarkerSync && SyncGroupInstance.ActivePlayers.IsValidIndex(SyncGroupInstance.GroupLeaderIndex))
		{
			return SyncGroupInstance.ActivePlayers[SyncGroupInstance.GroupLeaderIndex];
		}
	}
	return FAnimTickRecord();
}





/**
 * FSandboxFootPlacement
 */
FSandboxFootPlacement::FSandboxFootPlacement()
	: CachedUpperLimbIndex(INDEX_NONE)
	, CachedLowerLimbIndex(INDEX_NONE)
{
	bAllowStretch = false;
	InitStretchRatio = 0.95f;
	MaxStretchValue = 1.33f;
	bTakeRotationFromEffector = true;
	bKeepEffectorRelRot=false;
	bAllowBoneTwist = true;
	EffectorBoneSpace = EBoneControlSpace::BCS_BoneSpace;
	EffectorLocation = FVector::ZeroVector;
	JointTargetBoneSpace=EBoneControlSpace::BCS_BoneSpace;
	JointTargetLoc = FVector::ZeroVector;
	bDoFootPlacement=true;
	ResetInterpSpeed = 12.0f;
	FloorNormal = FVector::UpVector;
	MaxUpAdjustment=50.0f;
	MaxDownAdjustment = 45.0f;
	bOrientFootToGround = true;
	FootUpAxis = EFootPlacementAxis::EFP_X;
	bInvertFootUpAxis=true;
	MaxFootOrientAdjust=30.0f;
	MaxFootOrientFloorAngle=60.0f;
	FootSize=1.f;
	bProjectOntoPlane = true;
	PlaneNormal = EFootPlacementAxis::EFP_Z;
	bSkipIKWhenNotRendered = false;
	bEnforceMaxLimitUpdates = true;

#if WITH_EDITOR
	CachedJoints[0] = FVector::ZeroVector;
	CachedJoints[1] = FVector::ZeroVector;
	CachedJoints[2] = FVector::ZeroVector;
	CachedJoints[3] = FVector::ZeroVector;
#endif // WITH_EDITOR
}

static FVector GetBoneAxisDirVector(const FMatrix& BoneTransform, TEnumAsByte<EFootPlacementAxis> InAxis, bool bInvert)
{
	FVector AxisDir;
	switch (InAxis)
	{
	case EFootPlacementAxis::EFP_X:	
		AxisDir = BoneTransform.GetScaledAxis(EAxis::X);
		break;
	case EFootPlacementAxis::EFP_Y:	
		AxisDir = BoneTransform.GetScaledAxis(EAxis::Y);
		break;
	case EFootPlacementAxis::EFP_Z:	
		AxisDir = BoneTransform.GetScaledAxis(EAxis::Z);
		break;
	default:
		AxisDir = BoneTransform.GetScaledAxis(EAxis::Z);
		break;
	}
	if (bInvert)
	{
		AxisDir *= -1.f;
	}
	return AxisDir;
}

void FSandboxFootPlacement::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);
	EffectorBoneTarget.Initialize(Context.AnimInstanceProxy);
	JointBoneTarget.Initialize(Context.AnimInstanceProxy);
	AnimInstance = Cast<USandboxAnimation>(Context.AnimInstanceProxy->GetAnimInstanceObject());
	if (!AnimInstance)
	{
		UE_LOG(SandboxAnimLog, Error, TEXT("FSandboxFootPlacement::Initialize Error"))
	}
}

void FSandboxFootPlacement::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	FAnimNode_SkeletalControlBase::CacheBones_AnyThread(Context);
}

void FSandboxFootPlacement::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT(" IKFootBone: %s)"), *IKFootBone.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FSandboxFootPlacement::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);
	if (bDoFootPlacement && Alpha > KINDA_SMALL_NUMBER)
	{
		if (bEnforceMaxLimitUpdates && AnimInstance->GetWorld()->GetGameInstance() && AnimInstance->GetWorld()->GetGameInstance())
		{
			ASandboxGameModeBase* SandGame = Cast<ASandboxGameModeBase>(AnimInstance->GetWorld()->GetGameInstance());
			if (SandGame && SandGame->GetIsReplicated())
			{
				if (DoEnforceMaxLimitUpdates(SandGame))
				{
					if (Output.AnimInstanceProxy->GetSkelMeshComponent()->bRecentlyRendered || !bSkipIKWhenNotRendered)
					{
						EvaluateTwoBoneIK(Output, OutBoneTransforms);
						if (!EffectorRotation.IsZero())
						{
							ApplyEffectorRotation(OutBoneTransforms, EffectorRotation);
						}
					}
					return;
				}
				SandGame->NumFootPlacementUpdatesThisFrame++;
				FootPlacementUpdateTag = SandGame->FootPlacementUpdateTag;
			}
		}
		
		FMatrix const LocalToWorld = Output.AnimInstanceProxy->GetSkelMeshCompLocalToWorld().ToMatrixWithScale();
		USkeletalMeshComponent* const SkelMesh = Output.AnimInstanceProxy->GetSkelMeshComponent();	
		TargetEffectorLocation = FVector::ZeroVector;
		TargetEffectorRotation = FRotator::ZeroRotator;
		LegAdjust = 0.f;
		LegForwardAdjust = FVector::ZeroVector;
		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
		const FCompactPoseBoneIndex EffectorBoneIndex = IKFootBone.GetCompactPoseIndex(BoneContainer);
		FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(EffectorBoneIndex);
		FTransform const LocalFootOffset(FQuat::Identity, RelativeFootOffset);
		FTransform const EffectorToComponent = LocalFootOffset * EndBoneCSTransform;
		FVector EffectorLocComponent = EffectorToComponent.ToMatrixNoScale().GetOrigin();
		FVector EffectorLocWorld = LocalToWorld.TransformPosition(EffectorLocComponent);

#if WITH_EDITOR
		// White (IKFootBone): foot
		CachedFootPos = EffectorLocWorld;
#endif // WITH_EDITOR

		FVector ProjectionTranslation(FVector::ZeroVector);
		FVector PlaneNormalComponent(FVector::ZeroVector);
		FVector PlaneNormalWorld(FVector::ZeroVector);
		if (bProjectOntoPlane)
		{
			FCompactPoseBoneIndex PlaneBoneIndex = PlaneBone.GetCompactPoseIndex(BoneContainer);
			if (PlaneBoneIndex == INDEX_NONE)
			{
				FCompactPoseBoneIndex rootBone(0);
				PlaneBoneIndex = FCompactPoseBoneIndex(0);
			}
			FMatrix const PlaneToComponent = Output.Pose.GetComponentSpaceTransform(PlaneBoneIndex).ToMatrixNoScale();
			FVector const PlaneOriginWorld = LocalToWorld.TransformPosition(PlaneToComponent.GetOrigin());
			PlaneNormalComponent = GetBoneAxisDirVector(PlaneToComponent, PlaneNormal, bInvertPlaneNormal).GetSafeNormal();
			PlaneNormalWorld = LocalToWorld.TransformVector(PlaneNormalComponent);
			FVector const ProjectedEffectorLocWorld = FMath::LinePlaneIntersection(EffectorLocWorld, EffectorLocWorld + FloorNormal, PlaneOriginWorld, PlaneNormalWorld);
			ProjectionTranslation = ProjectedEffectorLocWorld - EffectorLocWorld;
			EffectorLocWorld = ProjectedEffectorLocWorld;
		}
		FVector IKLocWorldAdjustments = (MaxUpAdjustment)* FloorNormal;
		FVector CheckStartPos = EffectorLocWorld + IKLocWorldAdjustments;
		FVector const FootExtent = FVector(FootSize);
		FVector CheckEndPos = CheckStartPos - IKLocWorldAdjustments - (100.f + FootOffset + MaxDownAdjustment) * FloorNormal;
		FVector HitLocation(FVector::ZeroVector);
		FVector HitNormal(FVector::ZeroVector);
		bool bHit = LegLineCheck(SkelMesh->GetOwner(), CheckStartPos, CheckEndPos, HitLocation, HitNormal, FootExtent);
		if (bHit)
		{
			HitLocation -= HitNormal * FootExtent.Z;
			LegAdjust = ((EffectorLocWorld - HitLocation) | FloorNormal);
			if (LegAdjust > (FootOffset + MaxDownAdjustment))
			{
				LegAdjust = 0.f;
				bHit = false;
			}
		}
		FMatrix const WorldToLocalTM = LocalToWorld.InverseFast();
		if (bHit)
		{
			LegAdjust -= (FootOffset);
			LegAdjust = FMath::Clamp<float>(LegAdjust, -MaxUpAdjustment, MaxDownAdjustment);
			if (bProjectOntoPlane)
			{
				EffectorLocWorld -= ProjectionTranslation;
			}
			FVector DesiredLocWorld = EffectorLocWorld - (LegAdjust * FloorNormal);
			FVector DesiredLocLocal = WorldToLocalTM.TransformPosition(DesiredLocWorld);
			FMatrix ComponentToEffectorBT = EffectorToComponent.ToMatrixNoScale().Inverse();
			TargetEffectorLocation = ComponentToEffectorBT.TransformPosition(DesiredLocLocal);
		}

#if WITH_EDITOR
		DebugStartLoc = CheckStartPos;
		DebugEndLoc = CheckEndPos + FloorNormal * 100.f;
		DebugIKLoc = EffectorLocWorld;
		DebugHitLoc = HitLocation;
#endif

		if (Output.AnimInstanceProxy->GetSkelMeshComponent()->bRecentlyRendered || !bSkipIKWhenNotRendered || (GIsEditor))
		{
			EvaluateTwoBoneIK(Output, OutBoneTransforms);
		}

		if (bOrientFootToGround && PlaneBone.BoneName != EName::NAME_None)
		{
			if (bHit && !HitNormal.IsZero())
			{
				FVector NewFloorNormal = WorldToLocalTM.TransformVector(HitNormal);
				FQuat DeltaQuat = FQuat::FindBetween(PlaneNormalComponent, NewFloorNormal);
				FVector DeltaAxis(FVector::ZeroVector);
				float DeltaAngle = 0.f;
				DeltaQuat.ToAxisAndAngle(DeltaAxis, DeltaAngle);
				float MaxRotRadians = MaxFootOrientAdjust * (PI / 180.0f);
				if (FMath::Abs(DeltaAngle) > MaxRotRadians)
				{
					DeltaAngle = FMath::Clamp<float>(DeltaAngle, -MaxRotRadians, MaxRotRadians);
					DeltaQuat = FQuat(DeltaAxis, DeltaAngle);
				}
				const FCompactPoseBoneIndex IKBoneIndexRot = IKFootBone.GetCompactPoseIndex(BoneContainer);
				FQuat ComponentToPlaneQuat = Output.Pose.GetComponentSpaceTransform(IKBoneIndexRot).GetRotation();
				FQuat PlaneDeltaRotQuat = ComponentToPlaneQuat.Inverse() * DeltaQuat * ComponentToPlaneQuat;
				TargetEffectorRotation = PlaneDeltaRotQuat.Rotator();
			}
			if (Output.AnimInstanceProxy->GetSkelMeshComponent()->bRecentlyRendered || !bSkipIKWhenNotRendered || GIsEditor)
			{
				if (!EffectorRotation.IsZero())
				{
					ApplyEffectorRotation(OutBoneTransforms, EffectorRotation);
				}
			}
		}
	}
	else
	{
		TargetEffectorLocation = FVector::ZeroVector;
		TargetEffectorRotation = FRotator::ZeroRotator;
		LegAdjust = 0.f;
		if (Output.AnimInstanceProxy->GetSkelMeshComponent()->bRecentlyRendered || !bSkipIKWhenNotRendered || (GIsEditor))
		{
			EvaluateTwoBoneIK(Output, OutBoneTransforms);
		}
	}
}

bool FSandboxFootPlacement::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	if (!IKFootBone.IsValidToEvaluate(RequiredBones) || !PlaneBone.IsValidToEvaluate(RequiredBones))
	{
		return false;
	}

	if (EffectorBoneSpace == EBoneControlSpace::BCS_ParentBoneSpace || EffectorBoneSpace == EBoneControlSpace::BCS_BoneSpace)
	{
		if (!EffectorBoneTarget.IsValidToEvaluate(RequiredBones))
		{
			return false;
		}
	}

	if (JointTargetBoneSpace == EBoneControlSpace::BCS_ParentBoneSpace || JointTargetBoneSpace == EBoneControlSpace::BCS_BoneSpace)
	{
		if (!JointBoneTarget.IsValidToEvaluate(RequiredBones))
		{
			return false;
		}
	}

	if (CachedUpperLimbIndex == INDEX_NONE || CachedLowerLimbIndex == INDEX_NONE)
	{
		return false;
	}

	return true;
}

void FSandboxFootPlacement::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	IKFootBone.Initialize(RequiredBones);
	PlaneBone.Initialize(RequiredBones);
	EffectorBoneTarget.InitializeBoneReferences(RequiredBones);
	JointBoneTarget.InitializeBoneReferences(RequiredBones);
	const FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKFootBone.GetCompactPoseIndex(RequiredBones);
	CachedLowerLimbIndex = FCompactPoseBoneIndex(INDEX_NONE);
	CachedUpperLimbIndex = FCompactPoseBoneIndex(INDEX_NONE);
	if (IKBoneCompactPoseIndex != INDEX_NONE)
	{
		CachedLowerLimbIndex = RequiredBones.GetParentBoneIndex(IKBoneCompactPoseIndex);
		if (CachedLowerLimbIndex != INDEX_NONE)
		{
			CachedUpperLimbIndex = RequiredBones.GetParentBoneIndex(CachedLowerLimbIndex);
		}
	}
}

void FSandboxFootPlacement::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);
	InterpolatedEffectorLocation = FMath::VInterpConstantTo(InterpolatedEffectorLocation, TargetEffectorLocation, Context.GetDeltaTime(), ResetInterpSpeed);
	InterpolatedEffectorRotation = FMath::RInterpTo(InterpolatedEffectorRotation, TargetEffectorRotation, Context.GetDeltaTime(), ResetInterpSpeed * 256.0f);
	EffectorLocation = InterpolatedEffectorLocation * Alpha;
	EffectorRotation = InterpolatedEffectorRotation * Alpha;
	if (AnimInstance && Context.AnimInstanceProxy->GetSkelMeshComponent())
	{
		switch (FootLeg)
		{
		case EFootPlacementLeg::EFP_Left : AnimInstance->IKFootLeftLegAdjust = LegAdjust;
		case EFootPlacementLeg::EFP_Right: AnimInstance->IKFootRightLegAdjust = LegAdjust;
		}
	}
}

void FSandboxFootPlacement::EvaluateTwoBoneIK(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	bool bInvalidLimb = false;
	FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKFootBone.GetCompactPoseIndex(BoneContainer);
	const bool bInBoneSpace = (EffectorBoneSpace == BCS_ParentBoneSpace) || (EffectorBoneSpace == BCS_BoneSpace);
	const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
	const FTransform LowerLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedLowerLimbIndex);
	const FTransform UpperLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedUpperLimbIndex);
	FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex);
	FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex);
	FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();
	FTransform EffectorTransform = GetTargetTransform(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, EffectorBoneTarget, EffectorBoneSpace, EffectorLocation);
	FTransform JointTargetTransform = GetTargetTransform(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, JointBoneTarget, JointTargetBoneSpace, JointTargetLoc);
	FVector	JointTargetPos = JointTargetTransform.GetTranslation();
	FVector DesiredPos = EffectorTransform.GetTranslation();
	UpperLimbCSTransform.SetLocation(RootPos);
	LowerLimbCSTransform.SetLocation(InitialJointPos);
	EndBoneCSTransform.SetLocation(InitialEndPos);
	AnimationCore::SolveTwoBoneIK(UpperLimbCSTransform, LowerLimbCSTransform, EndBoneCSTransform, JointTargetPos, DesiredPos, bAllowStretch, InitStretchRatio, MaxStretchValue);
	if (!bAllowBoneTwist)
	{
		auto RemoveTwist = [this](const FTransform& InParentTransform, FTransform& InOutTransform, const FTransform& OriginalLocalTransform, const FVector& InAlignVector)
		{
			FTransform LocalTransform = InOutTransform.GetRelativeTransform(InParentTransform);
			FQuat LocalRotation = LocalTransform.GetRotation();
			FQuat NewTwist, NewSwing;
			LocalRotation.ToSwingTwist(InAlignVector, NewSwing, NewTwist);
			NewSwing.Normalize();

			// get new twist from old local
			LocalRotation = OriginalLocalTransform.GetRotation();
			FQuat OldTwist, OldSwing;
			LocalRotation.ToSwingTwist(InAlignVector, OldSwing, OldTwist);
			OldTwist.Normalize();

			InOutTransform.SetRotation(InParentTransform.GetRotation() * NewSwing * OldTwist);
			InOutTransform.NormalizeRotation();
		};

		const FCompactPoseBoneIndex UpperLimbParentIndex = BoneContainer.GetParentBoneIndex(CachedUpperLimbIndex);
		FVector AlignDir = TwistBoneAxis.GetTransformedAxis(FTransform::Identity);
		if (UpperLimbParentIndex != INDEX_NONE)
		{
			FTransform UpperLimbParentTransform = Output.Pose.GetComponentSpaceTransform(UpperLimbParentIndex);
			RemoveTwist(UpperLimbParentTransform, UpperLimbCSTransform, UpperLimbLocalTransform, AlignDir);
		}

		RemoveTwist(UpperLimbCSTransform, LowerLimbCSTransform, LowerLimbLocalTransform, AlignDir);
	}

	{
		OutBoneTransforms.Add(FBoneTransform(CachedUpperLimbIndex, UpperLimbCSTransform));
	}

	{
		OutBoneTransforms.Add(FBoneTransform(CachedLowerLimbIndex, LowerLimbCSTransform));
	}

	{
		if (bInBoneSpace && bTakeRotationFromEffector)
		{
			EndBoneCSTransform.SetRotation(EffectorTransform.GetRotation());
		}
		else if (bKeepEffectorRelRot)
		{
			EndBoneCSTransform = EndBoneLocalTransform * LowerLimbCSTransform;
		}
		OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));
	}
	check(OutBoneTransforms.Num() == 3);
}

FTransform FSandboxFootPlacement::GetTargetTransform(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases, FBoneSocketTarget& InTarget, EBoneControlSpace Space, const FVector& InOffset)
{
	FTransform OutTransform;
	if (Space == EBoneControlSpace::BCS_BoneSpace)
	{
		OutTransform = InTarget.GetTargetTransform(InOffset, MeshBases, InComponentTransform);
	}
	else
	{
		OutTransform.SetLocation(InOffset);
		FAnimationRuntime::ConvertBoneSpaceTransformToCS(InComponentTransform, MeshBases, OutTransform, InTarget.GetCompactPoseBoneIndex(), Space);
	}

	return OutTransform;
}

void FSandboxFootPlacement::ApplyEffectorRotation(TArray<FBoneTransform>& OutBoneTransforms, FRotator& DesiredEffectorRotation)
{
	OutBoneTransforms[2].Transform.ConcatenateRotation(DesiredEffectorRotation.Quaternion());
}

bool FSandboxFootPlacement::LegLineCheck(const AActor* Owner, const FVector& Start, const FVector& End, FVector& CurrentHitLocation, FVector& CurrentHitNormal, const FVector& Extent /*= FVector::ZeroVector*/)
{
	const UWorld* MyWorld = Owner->GetWorld();
	if (Owner && MyWorld)
	{
		FCollisionQueryParams QueryParams(TEXT("LegLineCheck"), true, Owner);
		TArray<FHitResult> MultiTraceHits;
		const bool bOutHit = Owner->GetWorld()->LineTraceMultiByChannel(MultiTraceHits, Start, End, ECollisionChannel::ECC_Visibility, QueryParams);
		if (bOutHit)
		{
			for (const FHitResult& HitResult : MultiTraceHits)
			{
				if (HitResult.bBlockingHit)
				{
					CurrentHitLocation = HitResult.Location;
					CurrentHitNormal = HitResult.ImpactNormal;
					return true;
				}
			}
		}
	}
	return false;
}

bool FSandboxFootPlacement::DoEnforceMaxLimitUpdates(ASandboxGameModeBase* SandGame)
{
	const float CurrentTime = AnimInstance->GetWorld()->GetTimeSeconds();
	if (SandGame->FootPlacementLineChecksTimeStamp != CurrentTime)
	{
		SandGame->FootPlacementLineChecksTimeStamp = CurrentTime;
		if (SandGame->NumFootPlacementUpdatesThisFrame < SandGame->MaxFootPlacementLineChecksPerFrame)
		{
			SandGame->FootPlacementUpdateTag++;
		}
		SandGame->NumFootPlacementUpdatesThisFrame = 0;
	}
	return (SandGame->NumFootPlacementUpdatesThisFrame < SandGame->MaxFootPlacementLineChecksPerFrame) && (FootPlacementUpdateTag != SandGame->FootPlacementUpdateTag);
}

#if WITH_EDITOR
void FSandboxFootPlacement::DrawDebugBones(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp) const
{
	DrawWireSphere(PDI, CachedFootPos, FLinearColor::White, 8.f, 8, SDPG_Foreground);
	DrawWireSphere(PDI, DebugStartLoc, FLinearColor::Red, 8.f, 8, SDPG_Foreground);
	DrawWireSphere(PDI, DebugEndLoc, FLinearColor::Green, 8.f, 8, SDPG_Foreground);
	DrawWireSphere(PDI, DebugIKLoc, FLinearColor::Yellow, 8.f, 8, SDPG_Foreground);
	DrawWireSphere(PDI, DebugHitLoc, FLinearColor::Blue, 8.f, 8, SDPG_Foreground);
	PDI->DrawLine(DebugStartLoc, DebugEndLoc, FLinearColor::Yellow, SDPG_Foreground);
}
#endif	//WITH_EDITOR
