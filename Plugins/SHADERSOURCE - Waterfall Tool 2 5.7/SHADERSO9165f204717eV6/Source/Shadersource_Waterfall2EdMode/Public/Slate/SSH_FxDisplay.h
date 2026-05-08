// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class USH_WaterfallSettingsComponent;

/* The widget to use in the Fx tab of the Ed Mode. */
class SHADERSOURCE_WATERFALL2EDMODE_API SSH_FxDisplay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSH_FxDisplay) {}
		SLATE_ATTRIBUTE(USH_WaterfallSettingsComponent*, Settings)
		SLATE_ATTRIBUTE(TSharedPtr<IPropertyHandle>, Prop_PathSelection)
		SLATE_ATTRIBUTE(FSimpleDelegate, RefreshSlate)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

private:
	USH_WaterfallSettingsComponent* Settings = nullptr;
	FSimpleDelegate RefreshSlate;

	FReply ButtonClicked_ChangeSelection(bool bIncrement);

	bool IsEnabled_SelectPrevious() const;
	bool IsEnabled_SelectNext() const;
};
