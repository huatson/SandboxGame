
#include "SandAnimGraphNodes.h"
#include "AnimNodeEditModes.h"

// for customization details
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"

// version handling
#include "AnimationCustomVersion.h"
//#include "ReleaseObjectVersion.h"

#define LOCTEXT_NAMESPACE "SandboxAnimationNodes"

USandAnimGraphNode_Movement::USandAnimGraphNode_Movement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Node.AddPose();	//	idle	
	Node.AddPose();	//	Walk
	Node.AddPose();	//	Run 
	Node.AddPose();	//	Roadie
}

FText USandAnimGraphNode_Movement::GetTooltipText() const
{
	return LOCTEXT("Movement_Tooltip", "Blend Movement Animation");
}

FText USandAnimGraphNode_Movement::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Movement_Title", "Movement");
}

FLinearColor USandAnimGraphNode_Movement::GetNodeTitleColor() const
{
	return FLinearColor(FColor::Red);
}

FString USandAnimGraphNode_Movement::GetNodeCategory() const
{
	return TEXT("ASANDBOX NODES");
}

void USandAnimGraphNode_Movement::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	FName BlendPoses(TEXT("BlendPose"));
	FName BlendTimes(TEXT("BlendTime"));

	if (ArrayIndex != INDEX_NONE)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("IdlePose"), LOCTEXT("Idle", "Idle"));
		Arguments.Add(TEXT("WalkPose"), LOCTEXT("Walk", "Walk"));
		Arguments.Add(TEXT("RunPose"), LOCTEXT("Run", "Run"));
		Arguments.Add(TEXT("RoadiePose"), LOCTEXT("RoadieRun", "RoadieRun"));

		if (SourcePropertyName == BlendPoses)
		{
			if (ArrayIndex == 0)
				Pin->PinFriendlyName = FText::Format(LOCTEXT("BlendPose_FriendlyName", "{IdlePose}"), Arguments);
			if (ArrayIndex == 1)
				Pin->PinFriendlyName = FText::Format(LOCTEXT("BlendPose_FriendlyName", "{WalkPose}"), Arguments);
			if (ArrayIndex == 2)
				Pin->PinFriendlyName = FText::Format(LOCTEXT("BlendPose_FriendlyName", "{RunPose}"), Arguments);
			if (ArrayIndex == 3)
				Pin->PinFriendlyName = FText::Format(LOCTEXT("BlendPose_FriendlyName", "{RoadiePose}"), Arguments);
		}
		else if (SourcePropertyName == BlendTimes)
		{
			Pin->SafeSetHidden(true);
		}
	}
}




/**
 * UAnimGraphNode_FootPlacement
 */
TSharedPtr<FFootPlacementDelegate> USandAnimGraphNode_Foot::FootPlacementDelegate = NULL;

USandAnimGraphNode_Foot::USandAnimGraphNode_Foot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//	Empty
}


FText USandAnimGraphNode_Foot::GetTooltipText() const
{
	return LOCTEXT("FootPlacement_ToolTip", "FootPlacement");
}

FLinearColor USandAnimGraphNode_Foot::GetNodeTitleColor() const
{
	return FLinearColor::Red;
}

FText USandAnimGraphNode_Foot::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.IKFootBone.BoneName == NAME_None))
	{
		return GetControllerDescription();
	}

	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
	else //if (!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		Args.Add(TEXT("BoneName"), FText::FromName(Node.IKFootBone.BoneName));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("FootPlacement_ListTitle", "{ControllerDescription} - Bone: {BoneName}"), Args), this);
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("FootPlacement_Title", "{ControllerDescription}\nBone: {BoneName}"), Args), this);
		}
	}

	return CachedNodeTitles[TitleType];
}

void USandAnimGraphNode_Foot::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	// initialize just once
	if (!FootPlacementDelegate.IsValid())
	{
		FootPlacementDelegate = MakeShareable(new FFootPlacementDelegate());
	}

	// do this first, so that we can include these properties first
	const FString EffectorBoneSpace = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, EffectorBoneSpace));
	const FString JointTargetBoneSpace = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, JointTargetBoneSpace));

	IDetailCategoryBuilder& IKCategory = DetailBuilder.EditCategory("IK");
	IDetailCategoryBuilder& EffectorCategory = DetailBuilder.EditCategory("Effector");
	IDetailCategoryBuilder& JointCategory = DetailBuilder.EditCategory("JointTarget");

	// refresh UIs when bone space is changed
	TSharedRef<IPropertyHandle> EffectorLocHandle = DetailBuilder.GetProperty(*EffectorBoneSpace, GetClass());
	if (EffectorLocHandle->IsValidHandle())
	{
		EffectorCategory.AddProperty(EffectorLocHandle);

		FSimpleDelegate UpdateEffectorSpaceDelegate = FSimpleDelegate::CreateSP(FootPlacementDelegate.Get(), &FFootPlacementDelegate::UpdateLocationSpace, &DetailBuilder);
		EffectorLocHandle->SetOnPropertyValueChanged(UpdateEffectorSpaceDelegate);
	}

	TSharedRef<IPropertyHandle> JointTragetLocHandle = DetailBuilder.GetProperty(*JointTargetBoneSpace, GetClass());
	if (JointTragetLocHandle->IsValidHandle())
	{
		JointCategory.AddProperty(JointTragetLocHandle);
		FSimpleDelegate UpdateJointSpaceDelegate = FSimpleDelegate::CreateSP(FootPlacementDelegate.Get(), &FFootPlacementDelegate::UpdateLocationSpace, &DetailBuilder);
		JointTragetLocHandle->SetOnPropertyValueChanged(UpdateJointSpaceDelegate);
	}

	EBoneControlSpace Space = Node.EffectorBoneSpace;
	const FString TakeRotationPropName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, bTakeRotationFromEffector));
	const FString EffectorTargetName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, EffectorBoneTarget));
	const FString EffectorLocationPropName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, EffectorLocation));

	if (Space == BCS_BoneSpace || Space == BCS_ParentBoneSpace)
	{
		TSharedPtr<IPropertyHandle> PropertyHandle;
		PropertyHandle = DetailBuilder.GetProperty(*TakeRotationPropName, GetClass());
		EffectorCategory.AddProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*EffectorTargetName, GetClass());
		EffectorCategory.AddProperty(PropertyHandle);
	}
	else // hide all properties in EndEffector category
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(*EffectorLocationPropName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*TakeRotationPropName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*EffectorTargetName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
	}

	Space = Node.JointTargetBoneSpace;
	bool bPinVisibilityChanged = false;
	const FString JointTargetName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, JointBoneTarget));
	const FString JointTargetLocation = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, JointTargetLoc));

	if (Space == BCS_BoneSpace || Space == BCS_ParentBoneSpace)
	{
		TSharedPtr<IPropertyHandle> PropertyHandle;
		PropertyHandle = DetailBuilder.GetProperty(*JointTargetName, GetClass());
		JointCategory.AddProperty(PropertyHandle);
	}
	else // hide all properties in JointTarget category except for JointTargetLocationSpace
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(*JointTargetName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
	}
}

FEditorModeID USandAnimGraphNode_Foot::GetEditorMode() const
{
	return AnimNodeEditModes::AnimNode;
}

void USandAnimGraphNode_Foot::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FSandboxFootPlacement* FootPlacement = static_cast<FSandboxFootPlacement*>(InPreviewNode);

	// copies Pin values from the internal node to get data which are not compiled yet
	FootPlacement->EffectorLocation = Node.EffectorLocation;
	FootPlacement->JointTargetLoc = Node.JointTargetLoc;
}

void USandAnimGraphNode_Foot::CopyPinDefaultsToNodeData(UEdGraphPin* InPin)
{
	if (InPin->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, EffectorLocation))
	{
		GetDefaultValue(GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, EffectorLocation), Node.EffectorLocation);
	}
	else if (InPin->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, JointTargetLoc))
	{
		GetDefaultValue(GET_MEMBER_NAME_STRING_CHECKED(FSandboxFootPlacement, JointTargetLoc), Node.JointTargetLoc);
	}
}

FString USandAnimGraphNode_Foot::GetNodeCategory() const
{
	return TEXT("ASANDBOX NODES");
}

void USandAnimGraphNode_Foot::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{
	if (bEnableDebugDraw && SkelMeshComp)
	{
		if (FSandboxFootPlacement* ActiveNode = GetActiveInstanceNode<FSandboxFootPlacement>(SkelMeshComp->GetAnimInstance()))
		{
			ActiveNode->DrawDebugBones(PDI, SkelMeshComp);
		}
	}
}

FText USandAnimGraphNode_Foot::GetControllerDescription() const
{
	return LOCTEXT("FootPlacement_Description", "FootPlacement");
}

#undef LOCTEXT_NAMESPACE
