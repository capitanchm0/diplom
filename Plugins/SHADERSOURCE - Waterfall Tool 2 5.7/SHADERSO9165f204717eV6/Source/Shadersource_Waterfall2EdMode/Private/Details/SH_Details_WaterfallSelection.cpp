// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Details/SH_Details_WaterfallSelection.h"
#include "Actors/SH_Waterfall2.h"
#include "SH_WaterfallTool2Statics.h"
#include "Slate/SH_W2_PlacementAssetEntry.h"
#include "Settings/SH_Waterfall2Settings.h"

#include "EditorClassUtils.h"

#include "Subsystems/UnrealEditorSubsystem.h"
#include "Kismet/GameplayStatics.h"

#include "Widgets/Layout/SScrollBox.h"

void FSH_Details_WaterfallSelection::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	if (GEditor)
	{
		UUnrealEditorSubsystem* UnrealSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
		if (UnrealSubsystem)
		{
			UWorld* EditorWorld = UnrealSubsystem->GetEditorWorld();
			if (EditorWorld)
			{
				FAssetData WaterfallAssetData = FAssetData(ASH_Waterfall2::StaticClass());

				IDetailCategoryBuilder& Cat_CreateNew = DetailBuilder.EditCategory("CreateNew", FText::GetEmpty(), ECategoryPriority::Default);
				Cat_CreateNew.AddCustomRow(FText::FromString("CreateNew"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSH_W2_PlacementAssetEntry)
						.TargetAsset(WaterfallAssetData)
						.ThumbnailPool(DetailBuilder.GetThumbnailPool())
						.AssetTitle(FText::FromString("SHADERSOURCE: Waterfall Tool 2"))
						.ToolTipText(FText::FromString("Drag and Drop into scene to create a SHADERSOURCE: Waterfall Tool 2 Actor"))
					]
					];


				TArray<AActor*> WaterfallActors = {};
				UGameplayStatics::GetAllActorsOfClass(EditorWorld, ASH_Waterfall2::StaticClass(), WaterfallActors);

				TSharedPtr<SWidget> CustomRowContent;
				if (WaterfallActors.Num() > 0)
				{
					TSharedPtr<SScrollBox> SelectionScrollBox = SNew(SScrollBox);

					for (AActor* WaterfallActor : WaterfallActors)
					{
						SelectionScrollBox->AddSlot()
							[
								SNew(SButton)
									.Text(FText::FromString(WaterfallActor->GetActorLabel()))
									.OnClicked(FOnClicked::CreateStatic(&FSH_Details_WaterfallSelection::ButtonClicked_SelectActor, WaterfallActor))
							];
					}

					CustomRowContent = SelectionScrollBox;
				}
				else
				{
					CustomRowContent = SNew(STextBlock)
						.Text(FText::FromString("There are no waterfalls currently in this level."))
						.AutoWrapText(true)
						.Justification(ETextJustify::Center);
				}

				IDetailCategoryBuilder& Cat_SelectWaterfall = DetailBuilder.EditCategory("SelectWaterfall", FText::GetEmpty(), ECategoryPriority::Default);
				Cat_SelectWaterfall.AddCustomRow(FText::FromString("AutoFocus"))
					[
						CreateAutoFocusCheckbox(CHB_AutoFocusWaterfall).ToSharedRef()
					];
				Cat_SelectWaterfall.AddCustomRow(FText::FromString("SelectWaterfall"))
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.VAlign(EVerticalAlignment::VAlign_Fill)
							[
								CustomRowContent.ToSharedRef()
							]
					];
			}
		}
	}
}

FReply FSH_Details_WaterfallSelection::ButtonClicked_SelectActor(AActor* ActorToSelect)
{
	SelectActor(ActorToSelect);

	return FReply::Handled();
}

void FSH_Details_WaterfallSelection::SelectActor(AActor* ActorToSelect)
{
	USH_Waterfall2Settings* Waterfall2ProjectSettings = GetMutableDefault<USH_Waterfall2Settings>();

	USH_WaterfallTool2Statics::SelectActorInViewport(ActorToSelect, Waterfall2ProjectSettings->bAutoFocusWaterfallOnSelection);
}

TSharedPtr<SWidget> FSH_Details_WaterfallSelection::CreateAutoFocusCheckbox(TSharedPtr<SCheckBox>& CheckboxToAssign, FMargin Padding)
{
	//Clear any references that are already made
	CheckboxToAssign.Reset();

	USH_Waterfall2Settings* Waterfall2ProjectSettings = GetMutableDefault<USH_Waterfall2Settings>();

	return SNew(SBox)
		.Padding(Padding)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			[
				SNew(STextBlock)
					.Text(FText::FromString("Auto Focus Waterfall"))
					.ToolTipText(FText::FromString("Toggle whether to auto focus a waterfall when selecting a Waterfall Actor through this menu."))
					.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			+ SHorizontalBox::Slot()
			[
				SAssignNew(CheckboxToAssign, SCheckBox)
				.IsChecked(Waterfall2ProjectSettings->bAutoFocusWaterfallOnSelection ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_UObject(Waterfall2ProjectSettings, &USH_Waterfall2Settings::UpdateAutoFocusWaterfall)
			]
		];
}