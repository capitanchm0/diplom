// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Slate/SSH_FxDisplay.h"
#include "Slate/SSH_FxPathDisplay.h"
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "EditorComponents/SH_WaterfallPathComponent.h"
#include "Actors/SH_Waterfall2.h"

void SSH_FxDisplay::Construct(const FArguments& InArgs)
{
	Settings = InArgs._Settings.Get();
	RefreshSlate = InArgs._RefreshSlate.Get();

	TSharedPtr<SHorizontalBox> HB_Paths = SNew(SHorizontalBox);

	//We only need to add 5 - two to the left, selected, and two to the right
	TArray<USH_WaterfallPathComponent*> Paths = Settings->GetParentWaterfall()->GetAllPathComponents();
	for (int i = -2; i <= 2; i++)
	{
		int32 PathIndex = Settings->FxPathSelected + i;

		if (PathIndex >= 0 && PathIndex < Paths.Num())
		{
			HB_Paths->AddSlot()
				.HAlign(EHorizontalAlignment::HAlign_Fill)
				[
					SNew(SSH_FxPathDisplay)
					.ParentWaterfall(Settings->GetParentWaterfall())
					.PathIndex(PathIndex)
					.IsEnabled(i == 0) //The middle one only should be active
				];
		}
		else
		{
			HB_Paths->AddSlot()
				.HAlign(EHorizontalAlignment::HAlign_Fill)
				[
					SNew(SBox)
				];
		}
	}

	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot() //Selection
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					.Text(FText::FromString("<"))
					.IsEnabled_Raw(this, &SSH_FxDisplay::IsEnabled_SelectPrevious)
					.OnClicked(this, &SSH_FxDisplay::ButtonClicked_ChangeSelection, false)
				]
				+ SHorizontalBox::Slot()
				[
					InArgs._Prop_PathSelection.Get()->CreatePropertyValueWidget(false)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					.Text(FText::FromString(">"))
					.IsEnabled_Raw(this, &SSH_FxDisplay::IsEnabled_SelectNext)
					.OnClicked(this, &SSH_FxDisplay::ButtonClicked_ChangeSelection, true)
				]
			]
			+ SVerticalBox::Slot() //Path
			[
				HB_Paths.ToSharedRef()
			]
		];
}

FReply SSH_FxDisplay::ButtonClicked_ChangeSelection(bool bIncrement)
{
	if (Settings)
	{
		if (bIncrement)
		{
			if (IsEnabled_SelectNext())
			{
				Settings->FxPathSelected++;
				RefreshSlate.ExecuteIfBound();
			}
		}
		else
		{
			if (IsEnabled_SelectPrevious())
			{
				Settings->FxPathSelected--;
				RefreshSlate.ExecuteIfBound();
			}
		}
	}

	return FReply::Handled();
}

bool SSH_FxDisplay::IsEnabled_SelectPrevious() const
{
	return Settings ? Settings->FxPathSelected - 1 >= 0 : false;
}

bool SSH_FxDisplay::IsEnabled_SelectNext() const
{
	if (Settings)
	{
		if (Settings->GetParentWaterfall())
		{
			return Settings->FxPathSelected + 1 < Settings->GetParentWaterfall()->NumPaths();
		}
	}

	return false;
}