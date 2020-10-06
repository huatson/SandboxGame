// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimGraphNode_BlendListBase.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "DetailLayoutBuilder.h"
#include "SandboxAnimation.h"
#include "SandAnimGraphNodes.generated.h"


/**
 * USandAnimGraphNode_Movement
 */
UCLASS(MinimalAPI)
class USandAnimGraphNode_Movement : public UAnimGraphNode_BlendListBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FSandboxMovement Node;

public:

	//~ Begin UEdGraphNode Interface.
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End UEdGraphNode Interface.

	//~ Begin UAnimGraphNode_Base Interface
	virtual FString GetNodeCategory() const override;
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	//~ End UAnimGraphNode_Base Interface

};



class FFootPlacementDelegate : public TSharedFromThis<FFootPlacementDelegate>
{
public:
	void UpdateLocationSpace(class IDetailLayoutBuilder* FootDetailBuilder)
	{
		if (FootDetailBuilder)
		{
			FootDetailBuilder->ForceRefreshDetails();
		}
	}
};



UCLASS()
class USandAnimGraphNode_Foot : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	FSandboxFootPlacement Node;

	/** Enable drawing of the debug information of the node */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bEnableDebugDraw;

	// just for refreshing UIs when bone space was changed
	static TSharedPtr<class FFootPlacementDelegate> FootPlacementDelegate;

private:

	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTitleTextTable CachedNodeTitles;

public:


	// UEdGraphNode interface
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_SkeletalControlBase interface
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
	// End of UAnimGraphNode_SkeletalControlBase interface

	// UAnimGraphNode_Base interface
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;
	/**
	* Override this function to push an editor mode when this node is selected
	* @return the editor mode to use when this node is selected
	*/
	virtual FEditorModeID GetEditorMode() const override;
	virtual void CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode) override;
	virtual void CopyPinDefaultsToNodeData(UEdGraphPin* InPin) override;
	// End of UAnimGraphNode_Base interface

protected:

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const override;
	// End of UAnimGraphNode_Base interface

	// UAnimGraphNode_SkeletalControlBase interface
	virtual void Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const override;
	virtual FText GetControllerDescription() const override;
	// End of UAnimGraphNode_SkeletalControlBase interface

private:


};