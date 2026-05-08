// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#include "EdMode/SH_Waterfall2EditorModeToolkit.h"
#include "EdMode/SH_Waterfall2EditorMode.h"
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "SH_WaterfallTool2Statics.h"
#include "Engine/Selection.h"
#include "Actors/SH_Waterfall2.h"

#include "Subsystems/EditorSubsystemBlueprintLibrary.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "Kismet/GameplayStatics.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Details/SH_Details_WaterfallSelection.h"
#include "EditorModeManager.h"
#include "ThumbnailRendering/ThumbnailManager.h"

#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SThrobber.h"
#include "Slate/SH_W2_PlacementAssetEntry.h"
#include "EdMode/SH_Waterfall2EditorModeStyle.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "SH_Waterfall2EditorModeToolkit"

FSH_Waterfall2EditorModeToolkit::FSH_Waterfall2EditorModeToolkit()
{
	ConstructShadersourceModesToolbarButtonStyle();
}

void FSH_Waterfall2EditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	DetailsPanel = USH_WaterfallTool2Statics::ConstructDetailsPanel(DetailsFocus, true);
	Commands = MakeShareable(new FUICommandList());


		SAssignNew(HeaderWidget, SOverlay)
		+ SOverlay::Slot()
		[
			SAssignNew(VB_HeaderWidget, SVerticalBox)
			+ SVerticalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.HAlign(EHorizontalAlignment::HAlign_Fill)
			.AutoHeight()
			[
				//Add the SHADERSOURCE Modes toolbar, and the documentation and store page buttons
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0.f, 0.f, 10.f, 0.f)
				.VAlign(EVerticalAlignment::VAlign_Center)
				.HAlign(EHorizontalAlignment::HAlign_Fill)
				[
					GenerateShadersourceModesToolbar().ToSharedRef()
				]
				+ SHorizontalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Center)
				.HAlign(EHorizontalAlignment::HAlign_Right)
				.AutoWidth()
				[
					USH_WaterfallTool2Statics::CreateDocumentationButton().ToSharedRef()
				]
				+ SHorizontalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Center)
				.HAlign(EHorizontalAlignment::HAlign_Right)
				.AutoWidth()
				[
					USH_WaterfallTool2Statics::CreateStorePageButton().ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSeparator)
				.Orientation(EOrientation::Orient_Horizontal)
				.Thickness(3.f)
			]
			//SHADERSOURCE LOGO AND LABEL
			+ SVerticalBox::Slot()
			.HAlign(EHorizontalAlignment::HAlign_Fill)
			.VAlign(EVerticalAlignment::VAlign_Center)
			.AutoHeight()
			[
				SNew(SOverlay)
					+ SOverlay::Slot()
					[
						//Add shadersource logo
						SNew(SImage)
							.Image(FSH_Waterfall2EditorModeStyle::Get().GetBrush("SHADERSOURCE.Title.Background"))
							//.RenderTransformPivot(FVector2D(1, 0))
							.DesiredSizeOverride(FVector2D(723 / 2, 126 / 2))
							.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
							.Visibility(EVisibility::HitTestInvisible)
					]
					+ SOverlay::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Left)
					.VAlign(EVerticalAlignment::VAlign_Bottom)
					[
						SNew(SImage)
							.Image(FSH_Waterfall2EditorModeStyle::Get().GetBrush("SHADERSOURCE.Watermark.Background"))
							//.RenderTransformPivot(FVector2D(1, 0))
							.DesiredSizeOverride(FVector2D(175/3, 201/3))
							.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
							.Visibility(EVisibility::HitTestInvisible)
					]
					+ SOverlay::Slot()
					.Padding(2.5, 0, 0, 0)
					.HAlign(EHorizontalAlignment::HAlign_Left)
					.VAlign(EVerticalAlignment::VAlign_Bottom)
					[
						//Shadersource name
						SNew(STextBlock)
						.Justification(ETextJustify::Left)
						.Text(FText::FromString("SHADERSOURCE.io"))
						.TextStyle(FSH_Waterfall2EditorModeStyle::Get(), "SHADERSOURCE.VersionText")
						.Visibility(EVisibility::HitTestInvisible)
						.ColorAndOpacity(FLinearColor(1, 1, 1, 0.4))

					]
					+ SOverlay::Slot()
					.Padding(0, 0, 0, 2.5)
					.HAlign(EHorizontalAlignment::HAlign_Right)
					.VAlign(EVerticalAlignment::VAlign_Bottom)
					[
						//Add tool version
						SNew(STextBlock)
						.Justification(ETextJustify::Left)
						.Text(FText::FromString("version " + USH_WaterfallTool2Statics::GetPluginVersion()))
						.TextStyle(FSH_Waterfall2EditorModeStyle::Get(), "SHADERSOURCE.VersionText")
						.Visibility(EVisibility::HitTestInvisible)
						.ColorAndOpacity(FLinearColor(1, 1, 1, 0.4))
					]
					+ SOverlay::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Center)
					.VAlign(EVerticalAlignment::VAlign_Center)
					[
						SNew(SImage)
							.Image(FSH_Waterfall2EditorModeStyle::Get().GetBrush("SHADERSOURCE.Title.Banner"))
							//.RenderTransformPivot(FVector2D(1, 0))
							.DesiredSizeOverride(FVector2D(603 / 1.75, 44 / 1.75))
							.ColorAndOpacity(FLinearColor(1, 1, 1, 1))
							.Visibility(EVisibility::HitTestInvisible)
					]
			]
			+ SVerticalBox::Slot()
			.Padding(0.f, 0.f, 0.f, 0.f)
			.AutoHeight()
			[
				SNew(SSeparator)
					.Orientation(Orient_Horizontal)
					.SeparatorImage(FSH_Waterfall2EditorModeStyle::Get().GetBrush("SHADERSOURCE.DropDown"))
					.Thickness(6.f)
					.ColorAndOpacity(FLinearColor(.01, .01, .01))
			]
			+ SVerticalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.HAlign(EHorizontalAlignment::HAlign_Fill)
			.AutoHeight()
			[
				SNew(SComboButton)
				.OnGetMenuContent(FOnGetContent::CreateRaw(this, &FSH_Waterfall2EditorModeToolkit::GenerateWaterfallsContextMenu))
				.Visibility(this, &FSH_Waterfall2EditorModeToolkit::ShowWaterfallSelection)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &FSH_Waterfall2EditorModeToolkit::GetWaterfallSelectionText)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			//.HAlign(EHorizontalAlignment::HAlign_Center)
			.VAlign(EVerticalAlignment::VAlign_Center)
			.Padding(0, 5, 0, 2.5)
			[
				SNew(SUniformWrapPanel)
				.SlotPadding(FMargin(0))
				.EvenRowDistribution(true)
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.Visibility(this, &FSH_Waterfall2EditorModeToolkit::ShowTabs)
				+ SUniformWrapPanel::Slot()
				[
					SNew(SButton)
					.IsEnabled(this, &FSH_Waterfall2EditorModeToolkit::IsNotState, ESH_W2_TabState::W2_TS_Simple)
					.OnClicked(this, &FSH_Waterfall2EditorModeToolkit::ButtonClicked_ChangeState, ESH_W2_TabState::W2_TS_Simple)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Simple"))
						.Justification(ETextJustify::Center)
					]
				]
				+ SUniformWrapPanel::Slot()
				[
					SNew(SButton)
					.IsEnabled(this, &FSH_Waterfall2EditorModeToolkit::IsNotStates, AdvancedTabs)
					.OnClicked(this, &FSH_Waterfall2EditorModeToolkit::ButtonClicked_AdvancedTab)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Advanced"))
						.Justification(ETextJustify::Center)
					]
				]
				+ SUniformWrapPanel::Slot()
				[
					SNew(SButton)
					.IsEnabled(this, &FSH_Waterfall2EditorModeToolkit::IsNotState, ESH_W2_TabState::W2_TS_Debug)
					.OnClicked(this, &FSH_Waterfall2EditorModeToolkit::ButtonClicked_ChangeState, ESH_W2_TabState::W2_TS_Debug)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Debug"))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SVerticalBox::Slot()
			.Padding(0, 2.5, 0, 1)
			.AutoHeight()
			[
				SNew(SSeparator)
					.Orientation(Orient_Horizontal)
					.SeparatorImage(FSH_Waterfall2EditorModeStyle::Get().GetBrush("SHADERSOURCE.DropDown"))
					.Thickness(6.f)
					.ColorAndOpacity(FLinearColor(.01, .01, .01))
					.Visibility(this, &FSH_Waterfall2EditorModeToolkit::ShowTabs)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				//Advanced Tab Buttons
				SNew(SOverlay)
				.Visibility(this, &FSH_Waterfall2EditorModeToolkit::ShowTabs_Advanced)
				+ SOverlay::Slot()
				.VAlign(EVerticalAlignment::VAlign_Fill)
				.HAlign(EHorizontalAlignment::HAlign_Fill)
				[
					SNew(SBorder)
					.Visibility(EVisibility::HitTestInvisible)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.BorderBackgroundColor(FSlateColor::UseForeground())
				]
				+ SOverlay::Slot()
				.Padding(2,2)
				[
					SNew(SUniformWrapPanel)
					.SlotPadding(FMargin(1.f))
					.EvenRowDistribution(true)
					.HAlign(EHorizontalAlignment::HAlign_Center)
					+ SUniformWrapPanel::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Fill)
					.VAlign(EVerticalAlignment::VAlign_Center)
					[
						SNew(SButton)
						.IsEnabled(this, &FSH_Waterfall2EditorModeToolkit::IsNotState, ESH_W2_TabState::W2_TS_Paths)
						.OnClicked(this, &FSH_Waterfall2EditorModeToolkit::ButtonClicked_ChangeState, ESH_W2_TabState::W2_TS_Paths)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.ContentPadding(FMargin(0, 5.0))
						.ContentScale(FVector2D(1, 1))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(EVerticalAlignment::VAlign_Bottom)
							.HAlign(EHorizontalAlignment::HAlign_Center)
							[
								SNew(SImage)
								.Image(FAppStyle::Get().GetBrush("LandscapeEditor.SplineTool"))
								.DesiredSizeOverride(FVector2D(16, 16))
								.Visibility(EVisibility::HitTestInvisible)
								
							]
							+ SVerticalBox::Slot()
							.VAlign(EVerticalAlignment::VAlign_Bottom)
							.HAlign(EHorizontalAlignment::HAlign_Center)
							.Padding(FMargin(0,2.5,0,0))
							[
								SNew(STextBlock)
								.Text(FText::FromString("Paths"))
								.Justification(ETextJustify::Center)
								.TextStyle(FSH_Waterfall2EditorModeStyle::Get(), "SHADERSOURCE.AdvancedButtonLabel")
							]
						]
					]
					+ SUniformWrapPanel::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Fill)
					.VAlign(EVerticalAlignment::VAlign_Center)
					[
						SNew(SButton)

						.IsEnabled(this, &FSH_Waterfall2EditorModeToolkit::IsNotState, ESH_W2_TabState::W2_TS_Meshes)
						.OnClicked(this, &FSH_Waterfall2EditorModeToolkit::ButtonClicked_ChangeState, ESH_W2_TabState::W2_TS_Meshes)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.ContentPadding(FMargin(0, 5.0))
						.ContentScale(FVector2D(1, 1))
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.VAlign(EVerticalAlignment::VAlign_Bottom)
								.HAlign(EHorizontalAlignment::HAlign_Center)
								.AutoHeight()
								[
									SNew(SImage)
										.Image(FAppStyle::Get().GetBrush("EditorViewport.CollisionVisibility"))
										.DesiredSizeOverride(FVector2D(16, 16))
										.Visibility(EVisibility::HitTestInvisible)

								]
								+ SVerticalBox::Slot()
								.VAlign(EVerticalAlignment::VAlign_Bottom)
								.HAlign(EHorizontalAlignment::HAlign_Center)
								.Padding(FMargin(0, 2.5, 0, 0))
								[
									SNew(STextBlock)
										.Text(FText::FromString("Meshes"))
										.Justification(ETextJustify::Center)
										.TextStyle(FSH_Waterfall2EditorModeStyle::Get(), "SHADERSOURCE.AdvancedButtonLabel")
								]
						]
					]
					+ SUniformWrapPanel::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Fill)
					.VAlign(EVerticalAlignment::VAlign_Center)
					[
						SNew(SButton)						
						.IsEnabled(this, &FSH_Waterfall2EditorModeToolkit::IsNotState, ESH_W2_TabState::W2_TS_FX)
						.OnClicked(this, &FSH_Waterfall2EditorModeToolkit::ButtonClicked_ChangeState, ESH_W2_TabState::W2_TS_FX)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.ContentPadding(FMargin(0, 5.0))
						.ContentScale(FVector2D(1, 1))
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.VAlign(EVerticalAlignment::VAlign_Bottom)
								.HAlign(EHorizontalAlignment::HAlign_Center)
								.AutoHeight()
								[
									SNew(SImage)
										.Image(FAppStyle::Get().GetBrush("PlacementBrowser.Icons.VisualEffects"))
										.DesiredSizeOverride(FVector2D(16, 16))
										.Visibility(EVisibility::HitTestInvisible)

								]
								+ SVerticalBox::Slot()
								.VAlign(EVerticalAlignment::VAlign_Bottom)
								.HAlign(EHorizontalAlignment::HAlign_Center)
								.Padding(FMargin(0, 2.5, 0, 0))
								[
									SNew(STextBlock)
										.Text(FText::FromString("FX"))
										.Justification(ETextJustify::Center)
										.TextStyle(FSH_Waterfall2EditorModeStyle::Get(), "SHADERSOURCE.AdvancedButtonLabel")
								]
						]
					]
					+ SUniformWrapPanel::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Fill)
					.VAlign(EVerticalAlignment::VAlign_Center)
					[
						SNew(SButton)
							.IsEnabled(this, &FSH_Waterfall2EditorModeToolkit::IsNotState, ESH_W2_TabState::W2_TS_Materials)
							.OnClicked(this, &FSH_Waterfall2EditorModeToolkit::ButtonClicked_ChangeState, ESH_W2_TabState::W2_TS_Materials)
							.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
							.ContentPadding(FMargin(0, 5.0))
							.ContentScale(FVector2D(1, 1))
							[
								SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									.VAlign(EVerticalAlignment::VAlign_Bottom)
									.HAlign(EHorizontalAlignment::HAlign_Center)
									.AutoHeight()
									[
										SNew(SImage)
											.Image(FAppStyle::Get().GetBrush("LandscapeEditor.AlphaBrush_Pattern"))
											.DesiredSizeOverride(FVector2D(16, 16))
											.Visibility(EVisibility::HitTestInvisible)

									]
									+ SVerticalBox::Slot()
									.VAlign(EVerticalAlignment::VAlign_Bottom)
									.HAlign(EHorizontalAlignment::HAlign_Center)
									.Padding(FMargin(0, 2.5, 0, 0))
									[
										SNew(STextBlock)
											.Text(FText::FromString("Materials"))
											.Justification(ETextJustify::Center)
											.TextStyle(FSH_Waterfall2EditorModeStyle::Get(), "SHADERSOURCE.AdvancedButtonLabel")
									]
							]
					]
					+ SUniformWrapPanel::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Fill)
					.VAlign(EVerticalAlignment::VAlign_Center)
					[
						SNew(SButton)			
						.IsEnabled(this, &FSH_Waterfall2EditorModeToolkit::IsNotState, ESH_W2_TabState::W2_TS_Bake)
						.OnClicked(this, &FSH_Waterfall2EditorModeToolkit::ButtonClicked_ChangeState, ESH_W2_TabState::W2_TS_Bake)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.ContentPadding(FMargin(0, 5.0))
						.ContentScale(FVector2D(1, 1))
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.VAlign(EVerticalAlignment::VAlign_Bottom)
								.HAlign(EHorizontalAlignment::HAlign_Center)
								.AutoHeight()
								[
									SNew(SImage)
										.Image(FAppStyle::Get().GetBrush("Persona.ConvertToStaticMesh"))
										.DesiredSizeOverride(FVector2D(16, 16))
										.Visibility(EVisibility::HitTestInvisible)

								]
								+ SVerticalBox::Slot()
								.VAlign(EVerticalAlignment::VAlign_Bottom)
								.HAlign(EHorizontalAlignment::HAlign_Center)
								.Padding(FMargin(0, 2.5, 0, 0))
								[
									SNew(STextBlock)
										.Text(FText::FromString("Bake"))
										.Justification(ETextJustify::Center)
										.TextStyle(FSH_Waterfall2EditorModeStyle::Get(), "SHADERSOURCE.AdvancedButtonLabel")
								]
						]
					]
				]
			]
			+ SVerticalBox::Slot().Padding(0,5,0,0)
			.AutoHeight()
			[
				SAssignNew(ActionButtonHolder, SBorder).Visibility(this, &FSH_Waterfall2EditorModeToolkit::ShowTabs)
			]
		]
		+ SOverlay::Slot()
		.HAlign(EHorizontalAlignment::HAlign_Center)
		.VAlign(EVerticalAlignment::VAlign_Bottom)
		[
			SNew(SVerticalBox)
			.Visibility(this, &FSH_Waterfall2EditorModeToolkit::ShowLoadingBar)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.Padding(0.f,0.f,0.f,10.f)
			[
				SNew(SCircularThrobber)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			[
				USH_WaterfallTool2Statics::CreateButton("Cancel", FOnClicked::CreateRaw(this, &FSH_Waterfall2EditorModeToolkit::ButtonClicked_Cancel), ESH_W2_ButtonStyle::W2_BS_Danger).ToSharedRef()
			]
		];

		SAssignNew(ToolkitWidget, SOverlay)
			+SOverlay::Slot()
			.VAlign(EVerticalAlignment::VAlign_Fill)
			[
				SAssignNew(VB_MainWidget, SVerticalBox)
					+ SVerticalBox::Slot()
					.VAlign(EVerticalAlignment::VAlign_Fill)
					[
						DetailsPanel.ToSharedRef()
					]
			];


	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

FName FSH_Waterfall2EditorModeToolkit::GetToolkitFName() const
{
	return FName("SH_Waterfall2EditorMode");
}

FText FSH_Waterfall2EditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "SHADERSOURCE: Waterfall Tool 2");
}

void FSH_Waterfall2EditorModeToolkit::InvokeUI()
{
	FModeToolkit::InvokeUI();

	if (ModeToolHeader.IsValid()) ModeToolHeader->SetContent(HeaderWidget.ToSharedRef());

}

void FSH_Waterfall2EditorModeToolkit::UpdateDetailsFocus(TObjectPtr<UObject> NewSettingsObject)
{
	if (NewSettingsObject != DetailsFocus)
	{
		if (USH_WaterfallSettingsComponent* PrevWaterfallSettings = Cast<USH_WaterfallSettingsComponent>(DetailsFocus))
		{
			//Clear the UI enable delegate in the previous waterfall
			PrevWaterfallSettings->GetParentWaterfall()->OnEnableUI.Unbind();
		}

		DetailsFocus = NewSettingsObject;
		if (DetailsPanel.IsValid()) DetailsPanel->SetObject(DetailsFocus, true);

		if (USH_WaterfallSettingsComponent* WaterfallSettings = Cast<USH_WaterfallSettingsComponent>(DetailsFocus))
		{
			WaterfallSettings->RefreshDetailsPanel = FSimpleDelegate::CreateRaw(this, &FSH_Waterfall2EditorModeToolkit::RefreshDetailsPanel);
			bFocusedWaterfall = true;
			ButtonClicked_ChangeState(WaterfallSettings->GetCurrentTabState());
		}
		else
		{
			bFocusedWaterfall = false;
			if (ActionButtonHolder.IsValid()) ActionButtonHolder->ClearContent();
		}
	}
}

void FSH_Waterfall2EditorModeToolkit::SetTookitWidgetEnabled(bool bEnabled, FSimpleDelegate OptionalCancelDelegate)
{
	bMainWidgetEnabled = bEnabled;

	if (VB_MainWidget.IsValid()) VB_MainWidget->SetEnabled(bEnabled);
	if (VB_HeaderWidget.IsValid()) VB_HeaderWidget->SetEnabled(bEnabled);

	if (bEnabled) ClearCancelDelegate();
	else SetCancelDelegate(OptionalCancelDelegate);
}

FReply FSH_Waterfall2EditorModeToolkit::ButtonClicked_ChangeState(ESH_W2_TabState NewState)
{
	if (USH_WaterfallSettingsComponent* WaterfallSettings = Cast<USH_WaterfallSettingsComponent>(DetailsFocus))
	{
		WaterfallSettings->UpdateCurrentTabState(NewState);
		ActionButtonHolder->ClearContent();
		ActionButtonHolder->SetContent(WaterfallSettings->GetTabActionWidget().ToSharedRef());
		RefreshDetailsPanel();
	}

	return FReply::Handled();
}

FReply FSH_Waterfall2EditorModeToolkit::ButtonClicked_AdvancedTab()
{
	if (USH_WaterfallSettingsComponent* WaterfallSettings = Cast<USH_WaterfallSettingsComponent>(DetailsFocus))
	{
		return ButtonClicked_ChangeState(WaterfallSettings->HasGeneratedMeshes() ? ESH_W2_TabState::W2_TS_Meshes : ESH_W2_TabState::W2_TS_Paths);
	}

	return FReply::Handled();
}

void FSH_Waterfall2EditorModeToolkit::SetCancelDelegate(FSimpleDelegate NewCancelAction)
{
	Del_Cancel = NewCancelAction;
}

void FSH_Waterfall2EditorModeToolkit::ClearCancelDelegate()
{
	Del_Cancel.Unbind();
}

void FSH_Waterfall2EditorModeToolkit::ConstructShadersourceModesToolbarButtonStyle()
{
	//Get the default button style and set the padding to 0.f so that the images use the content padding instead
	ButtonStyle_NoPadding = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
	ButtonStyle_NoPadding.NormalPadding = FMargin(0.f);
	ButtonStyle_NoPadding.PressedPadding = FMargin(0.f);
}

TSharedPtr<SWidget> FSH_Waterfall2EditorModeToolkit::GenerateEdModeButton(FEditorModeInfo EdModeInfo)
{
	return SNew(SBox)
		.MaxDesiredHeight(30.f)
		.MaxDesiredWidth(30.f)
		[
			SNew(SButton)
			.ToolTipText(EdModeInfo.Name)
			.OnClicked(FOnClicked::CreateStatic(&USH_WaterfallTool2Statics::ButtonClicked_InvokeEditorMode, EdModeInfo.ID))
			.ContentPadding(5.f)
			.ButtonStyle(&ButtonStyle_NoPadding)
			.IsEnabled(EdModeInfo.ID != USH_Waterfall2EditorMode::EM_SH_Waterfall2EditorModeId)
			[
				SNew(SImage)
				.Image(EdModeInfo.IconBrush.GetIcon())
				.DesiredSizeOverride(FVector2D(25.f))
			]
		];
}

TSharedPtr<SWidget> FSH_Waterfall2EditorModeToolkit::GenerateShadersourceModesToolbar(TArray<FString> Filters)
{
	TSharedPtr<SScrollBox> ToolsScrollbox = SNew(SScrollBox)
		.Orientation(EOrientation::Orient_Horizontal);

	UEditorSubsystem* BaseEditorSubsystem = UEditorSubsystemBlueprintLibrary::GetEditorSubsystem(UAssetEditorSubsystem::StaticClass());
	if (UAssetEditorSubsystem* AssetEditorSubsystem = Cast<UAssetEditorSubsystem>(BaseEditorSubsystem))
	{
		TArray<FEditorModeInfo> ShadersourceModes = {};
		for (FEditorModeInfo EdModeInfo : AssetEditorSubsystem->GetEditorModeInfoOrderedByPriority())
		{
			int32 FilterCount = 0;
			for (FString Filter : Filters)
			{
				if (EdModeInfo.ID.ToString().Contains(Filter))
				{
					FilterCount++;
				}
				else break; //If missing one then we don't need to continue because we want to check that the mode ID contains ALL filters
			}

			if (FilterCount == Filters.Num())
			{
				ShadersourceModes.Add(EdModeInfo);
			}
		}

		ShadersourceModes.Sort([](const FEditorModeInfo& A, const FEditorModeInfo& B) {
			return A.Name.ToString() < B.Name.ToString(); });

		for (FEditorModeInfo ShadersourceMode : ShadersourceModes)
		{
			ToolsScrollbox->AddSlot()
				[
					GenerateEdModeButton(ShadersourceMode).ToSharedRef()
				];
		}
	}

	return ToolsScrollbox;
}

TSharedRef<SWidget> FSH_Waterfall2EditorModeToolkit::GenerateWaterfallsContextMenu()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	const FName ContextMenuName = "WaterfallTool2SelectionContextMenu";
	if (!ToolMenus->IsMenuRegistered(ContextMenuName))
	{
		ToolMenus->RegisterMenu(ContextMenuName);
	}
	UToolMenu* Menu = ToolMenus->FindMenu(ContextMenuName);

	FToolMenuSection& SectionA = Menu->AddSection("CreateNew", FText::FromString("Create New"));
	{
		FAssetData WaterfallAssetData = FAssetData(ASH_Waterfall2::StaticClass());

		SectionA.AddEntry(FToolMenuEntry::InitWidget("CreateNewWaterfall",
			SNew(SSH_W2_PlacementAssetEntry)
			.TargetAsset(WaterfallAssetData)
			.ThumbnailPool(UThumbnailManager::Get().GetSharedThumbnailPool())
			.AssetTitle(FText::FromString("SHADERSOURCE: Waterfall Tool 2"))
			.ToolTipText(FText::FromString("Drag and Drop into scene to create a SHADERSOURCE: Waterfall Tool 2 Actor")),
			FText(), true, false));
	}

	FToolMenuSection& SectionC = Menu->AddSection("Options", FText::FromString("Settings"));
	SectionC.AddEntry(FToolMenuEntry::InitWidget("AutoFocus",
		FSH_Details_WaterfallSelection::CreateAutoFocusCheckbox(CHB_AutoFocusWaterfall, FMargin(27.f, 0.f, 0.f, 0.f)).ToSharedRef(),
		FText(), false, false));

	FToolMenuSection& SectionB = Menu->AddSection("WaterfallAssets", FText::FromString("Select in Level"));

	if (GEditor)
	{
		UUnrealEditorSubsystem* UnrealSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
		if (UnrealSubsystem)
		{
			UWorld* EditorWorld = UnrealSubsystem->GetEditorWorld();
			if (EditorWorld)
			{
				TArray<AActor*> WaterfallActors = {};
				UGameplayStatics::GetAllActorsOfClass(EditorWorld, ASH_Waterfall2::StaticClass(), WaterfallActors);

				for(AActor* WaterfallActor : WaterfallActors)
				{
					SectionB.AddMenuEntry(WaterfallActor->GetFName(),
						FText::FromString(WaterfallActor->GetActorLabel()),
						FText::FromString("Select this waterfall."),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateStatic(&FSH_Details_WaterfallSelection::SelectActor, WaterfallActor),
							FCanExecuteAction()
						)
					);
				}
			}
		}
	}

	FToolMenuContext MenuContext(Commands, nullptr, nullptr);
	return ToolMenus->GenerateWidget(ContextMenuName, MenuContext);
}

FText FSH_Waterfall2EditorModeToolkit::GetWaterfallSelectionText() const
{
	if (USH_WaterfallSettingsComponent* WaterfallSettings = Cast<USH_WaterfallSettingsComponent>(DetailsFocus))
	{
		if (WaterfallSettings->GetParentWaterfall())
		{
			return FText::FromString(WaterfallSettings->GetParentWaterfall()->GetActorLabel());
		}
	}

	return NoWaterfallSelected;
}

bool FSH_Waterfall2EditorModeToolkit::IsState(ESH_W2_TabState ThisState) const
{
	if (USH_WaterfallSettingsComponent* WaterfallSettings = Cast<USH_WaterfallSettingsComponent>(DetailsFocus))
	{
		return ThisState == WaterfallSettings->GetCurrentTabState();
	}

	return false;
}

bool FSH_Waterfall2EditorModeToolkit::IsNotState(ESH_W2_TabState ThisState) const
{
	return !IsState(ThisState);
}

bool FSH_Waterfall2EditorModeToolkit::IsNotStates(TArray<ESH_W2_TabState> TheseStates) const
{
	for (ESH_W2_TabState ThisState : TheseStates)
	{
		if (IsState(ThisState)) return false;
	}

	return true;
}

EVisibility FSH_Waterfall2EditorModeToolkit::ShowTabs() const
{
	if (USH_WaterfallSettingsComponent* WaterfallSettings = Cast<USH_WaterfallSettingsComponent>(DetailsFocus))
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FSH_Waterfall2EditorModeToolkit::ShowTabs_Advanced() const
{
	bool bOneVisible = false;
	for (ESH_W2_TabState AdvancedState : AdvancedTabs)
	{
		if (IsState(AdvancedState))
		{
			bOneVisible = true;
			break;
		}
	}

	return bOneVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FSH_Waterfall2EditorModeToolkit::ShowLoadingBar() const
{
	return bMainWidgetEnabled ? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible;
}

EVisibility FSH_Waterfall2EditorModeToolkit::ShowWaterfallSelection() const
{
	return bFocusedWaterfall ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply FSH_Waterfall2EditorModeToolkit::ButtonClicked_Cancel()
{
	Del_Cancel.ExecuteIfBound();

	return FReply::Handled();
}

void FSH_Waterfall2EditorModeToolkit::RefreshDetailsPanel()
{
	DetailsPanel->ForceRefresh();
}

#undef LOCTEXT_NAMESPACE