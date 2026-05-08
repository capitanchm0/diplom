// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class ASH_Waterfall2;
class USH_WaterfallVFXComponent;
class ISH_WaterfallFxComponent;

enum ESH_W2_FxType
{
	VFX,
	SFX,
};

/* The widget to show on the menu popup when clicking on a point in the Fx Tab of the Ed Mode. */
class SHADERSOURCE_WATERFALL2EDMODE_API SSH_FxPointDisplay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSH_FxPointDisplay) {}
		SLATE_ATTRIBUTE(FVector2D, WindowSize)
		SLATE_ATTRIBUTE(int32, PathIndex)
		SLATE_ATTRIBUTE(int32, PointIndex)
		SLATE_ATTRIBUTE(ASH_Waterfall2*, ParentWaterfall)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	static ISH_WaterfallFxComponent* AddComponent_FX(ASH_Waterfall2* InWaterfall, int32 InPathIndex, int32 InPointIndex, ESH_W2_FxType FxType);

private:
	ASH_Waterfall2* ParentWaterfall = nullptr;
	int32 PathIndex = 0;
	int32 PointIndex = 0;
	int32 TargetIndex = INDEX_NONE;
	TSharedPtr<IDetailsView> DetailsPanel;
	FText FocusComponentName = FText::GetEmpty();

	ISH_WaterfallFxComponent* GetTargetFxComp();

	FReply ButtonClicked_AddFX(ESH_W2_FxType FxType);
	FReply ButtonClicked_DeleteFX();
	FReply ButtonClicked_ChangeFocus(bool bLeft);
	FReply ButtonClicked_ShiftPointLocation(bool bUp);

	void UpdateFocus();

	EVisibility GetVis_HasFX() const;

	bool IsEnabled_FxFocusButton(bool bLeft) const;
	bool IsEnabled_CanShift(bool bUp);

	FText GetText_FocusedComponent() const;

	TArray<ISH_WaterfallFxComponent*> TargetComponents = {};
	void UpdateTargetComponents();
};
