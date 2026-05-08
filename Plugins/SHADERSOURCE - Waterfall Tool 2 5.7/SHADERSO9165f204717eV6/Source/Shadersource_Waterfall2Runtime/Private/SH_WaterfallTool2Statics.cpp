// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "SH_WaterfallTool2Statics.h"
#include "Components/SplineComponent.h"

#if WITH_EDITOR
#include "Widgets/Notifications/SNotificationList.h" //Used in PrintEditorNotification()
#include "Framework/Notifications/NotificationManager.h" //Used in PrintEditorNotification()
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "UnrealEdGlobals.h" //For GUnrealEd
#include "Editor/UnrealEdEngine.h" //For UUnrealEdEngine for GUnrealEd
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "EditorDirectories.h"
#include "Interfaces/IPluginManager.h"
#endif

#include "AssetRegistry/AssetRegistryModule.h" //For getting Asset Registry
#include "AssetRegistry/AssetData.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#if WITH_EDITOR
TSharedPtr<SNotificationItem> USH_WaterfallTool2Statics::PrintEditorNotification(FText inText, ESH_W2_NotificationState State, bool bUseSuccessFailIcons, bool bFireAndForget, float FadeInDuration, float FadeOutDuration, float ExpireDuration, bool bUseLargeFont, bool bUseThrobber, bool bAllowThrottleWhenFrameRateIsLow)
{
	FNotificationInfo NewInfo(inText);
	NewInfo.FadeInDuration = FadeInDuration;
	NewInfo.FadeOutDuration = FadeOutDuration;
	NewInfo.ExpireDuration = ExpireDuration;
	NewInfo.bUseThrobber = bUseThrobber;
	NewInfo.bUseSuccessFailIcons = bUseSuccessFailIcons;
	NewInfo.bUseLargeFont = bUseLargeFont;
	NewInfo.bFireAndForget = bFireAndForget;
	NewInfo.bAllowThrottleWhenFrameRateIsLow = bAllowThrottleWhenFrameRateIsLow;

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(NewInfo);

	//Determines whether the message is a success or failure message
	switch (State)
	{
	case ESH_W2_NotificationState::W2_NS_Success: NotificationItem->SetCompletionState(SNotificationItem::CS_Success); break;
	case ESH_W2_NotificationState::W2_NS_Fail: NotificationItem->SetCompletionState(SNotificationItem::CS_Fail); break;
	case ESH_W2_NotificationState::W2_NS_Pending: NotificationItem->SetCompletionState(SNotificationItem::CS_Pending); break;
	case ESH_W2_NotificationState::W2_NS_None: NotificationItem->SetCompletionState(SNotificationItem::CS_None); break;
	}

	PrintMessageToLog(inText);

	return NotificationItem;
}
#endif

void USH_WaterfallTool2Statics::PrintMessageToLog(FString message, ELogVerbosity::Type Verbosity)
{
	switch (Verbosity)
	{
	case ELogVerbosity::Error: UE_LOG(LogTemp, Error, TEXT("%s"), *message); break;
	case ELogVerbosity::Display: UE_LOG(LogTemp, Display, TEXT("%s"), *message); break;
	case ELogVerbosity::Warning: UE_LOG(LogTemp, Warning, TEXT("%s"), *message); break;
	case ELogVerbosity::Fatal: UE_LOG(LogTemp, Fatal, TEXT("%s"), *message); break;
	default: UE_LOG(LogTemp, Log, TEXT("%s"), *message); break;
	};
}

#if WITH_EDITOR
void USH_WaterfallTool2Statics::InvokeEditorMode(FEditorModeID EdModeID)
{
	//Referenced from [SLevelEditor::ToggleEditorMode]

	// Abort viewport tracking when switching editor mode
	if (GCurrentLevelEditingViewportClient)
	{
		GCurrentLevelEditingViewportClient->AbortTracking();
	}
	
	// *Important* - activate the mode first since FEditorModeTools::DeactivateMode will
	// activate the default mode when the stack becomes empty, resulting in multiple active visible modes.
	GLevelEditorModeTools().ActivateMode(EdModeID);
}

FReply USH_WaterfallTool2Statics::ButtonClicked_InvokeEditorMode(FEditorModeID EdModeID)
{
	InvokeEditorMode(EdModeID);

	return FReply::Handled();
}
#endif

TSharedPtr<SButton> USH_WaterfallTool2Statics::CreateButton(FString Text, FOnClicked OnClicked, ESH_W2_ButtonStyle Style, bool bDisable, FString Tooltip)
{
	if (Style != ESH_W2_ButtonStyle::W2_BS_Default)
	{
		FName StyleName = FName("FlatButton.Primary");
		switch (Style)
		{
		case ESH_W2_ButtonStyle::W2_BS_Success:	StyleName = FName("FlatButton.Success"); break;
		case ESH_W2_ButtonStyle::W2_BS_Info:	StyleName = FName("FlatButton.Info"); break;
		case ESH_W2_ButtonStyle::W2_BS_Warning:	StyleName = FName("FlatButton.Warning"); break;
		case ESH_W2_ButtonStyle::W2_BS_Danger:	StyleName = FName("FlatButton.Danger"); break;
		}

		return SNew(SButton)
			.Text(FText::FromString(Text))
			.ToolTipText(FText::FromString(Tooltip))
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.VAlign(EVerticalAlignment::VAlign_Center)
			.ButtonStyle(FAppStyle::Get(), StyleName)
			.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
			.OnClicked(OnClicked)
			.IsEnabled(!bDisable);
	}

	return SNew(SButton)
		.Text(FText::FromString(Text))
		.ToolTipText(FText::FromString(Tooltip))
		.HAlign(EHorizontalAlignment::HAlign_Center)
		.VAlign(EVerticalAlignment::VAlign_Center)
		.OnClicked(OnClicked)
		.IsEnabled(!bDisable);
}

#if WITH_EDITOR
TSharedPtr<SButton> USH_WaterfallTool2Statics::AddBasicButtonRow(IDetailCategoryBuilder& InCategory, FString Text, FOnClicked OnClicked, ESH_W2_ButtonStyle Style, bool bDisable, FString Tooltip, bool bValueContentOnly)
{
	TSharedPtr<SButton> ReturnValue = CreateButton(Text, OnClicked, Style, bDisable, Tooltip);

	if (bValueContentOnly)
	{
		InCategory.AddCustomRow(FText::FromString(Text))
			.ValueContent()
			[
				ReturnValue.ToSharedRef()
			];
	}
	else
	{
		InCategory.AddCustomRow(FText::FromString(Text))
			[
				ReturnValue.ToSharedRef()
			];
	}
	
	return ReturnValue;
}

TSharedPtr<IDetailsView> USH_WaterfallTool2Statics::ConstructDetailsPanel(UObject* SettingsObject, bool bAllowSearch, bool bShowObjectLabel)
{
	TArray<UObject*> TempArray = { SettingsObject };
	return ConstructDetailsPanel(TempArray, bAllowSearch, bShowObjectLabel);
}

TSharedPtr<IDetailsView> USH_WaterfallTool2Statics::ConstructDetailsPanel(TArray<UObject*> SettingsObjects, bool bAllowSearch, bool bShowObjectLabel)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs = FDetailsViewArgs();
	DetailsViewArgs.bAllowSearch = bAllowSearch;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.bShowObjectLabel = bShowObjectLabel;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = false;
	DetailsViewArgs.bCustomFilterAreaLocation = false;
	DetailsViewArgs.bShowSectionSelector = false;

	TSharedPtr<IDetailsView> DetailsPanel = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsPanel->SetObjects(SettingsObjects);
	
	return DetailsPanel;
}

TSharedPtr<SWidget> USH_WaterfallTool2Statics::ConstructBasicDetailsPanelWidget(TSharedPtr<IDetailsView> InDetailsPanel, FString DocumentationURL, FString StorePageURL, TSharedPtr<SWidget> AdditionalHeaderContent)
{
	TSharedPtr<SVerticalBox> HeaderBox = SNew(SVerticalBox);
	TSharedPtr<SHorizontalBox> HeaderButtonBox = SNew(SHorizontalBox);
	HeaderBox->AddSlot()
		.VAlign(EVerticalAlignment::VAlign_Top)
		.HAlign(EHorizontalAlignment::HAlign_Right)
		[
			HeaderButtonBox.ToSharedRef()
		];

	if (AdditionalHeaderContent.IsValid())
	{
		HeaderButtonBox->AddSlot()
			.VAlign(EVerticalAlignment::VAlign_Top)
			.HAlign(EHorizontalAlignment::HAlign_Right)
			.AutoWidth()
			[
				AdditionalHeaderContent.ToSharedRef()
			];
	}

	if (!DocumentationURL.IsEmpty())
	{
		HeaderButtonBox->AddSlot()
			.VAlign(EVerticalAlignment::VAlign_Top)
			.HAlign(EHorizontalAlignment::HAlign_Right)
			.AutoWidth()
			[
				CreateDocumentationButton(DocumentationURL).ToSharedRef()
			];
	}

	//Store Page Button
	HeaderButtonBox->AddSlot()
		.VAlign(EVerticalAlignment::VAlign_Top)
		.HAlign(EHorizontalAlignment::HAlign_Right)
		.AutoWidth()
		[
			CreateStorePageButton(StorePageURL).ToSharedRef()
		];

	HeaderBox->AddSlot()
		.AutoHeight()
		[
			InDetailsPanel->GetFilterAreaWidget().ToSharedRef()
		];

	return 	SNew(SBox)
			.HAlign(EHorizontalAlignment::HAlign_Fill)
			.VAlign(EVerticalAlignment::VAlign_Fill)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 2.f)
					[
						HeaderBox.ToSharedRef()
					]
				+ SVerticalBox::Slot()
					[
						InDetailsPanel.ToSharedRef()
					]
			];
}
#endif

TSharedPtr<SButton> USH_WaterfallTool2Statics::CreateURLButton(FString URL, FText ButtonText, FText TooltipText, TSharedPtr<SImage> ButtonImage)
{
	TSharedPtr<SHorizontalBox> HB_ButtonContent = SNew(SHorizontalBox);
	
	if (ButtonImage.IsValid())
	{
		HB_ButtonContent->AddSlot()
			.AutoWidth()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.Padding(2, 0)
			[
				ButtonImage.ToSharedRef()
			];
	}

	if (!ButtonText.IsEmpty())
	{
		HB_ButtonContent->AddSlot()
			.AutoWidth()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.Padding(2, 0)
			[
				SNew(STextBlock)
				.Text(ButtonText)
				.Justification(ETextJustify::Center)
				.TextStyle(&FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("ButtonText"))
			];
	}

	return SNew(SButton)
		.ToolTipText(TooltipText)
		.OnClicked(FOnClicked::CreateStatic(&USH_WaterfallTool2Statics::ButtonClicked_OpenURL, URL))
		[
			HB_ButtonContent.ToSharedRef()
		];
}

#if WITH_EDITOR
TSharedPtr<SButton> USH_WaterfallTool2Statics::CreateDocumentationButton(FString DocumentationURL, FString Tooltip)
{
	return CreateURLButton(
		(DocumentationURL.IsEmpty()) ? GetPluginDocsLink() : DocumentationURL, FText::FromString("Documentation"),
		FText::FromString((Tooltip.IsEmpty()) ? "Open documentation for this tool!" : Tooltip),
		SNew(SImage).Image(FAppStyle::Get().GetBrush("Icons.Documentation"))
	);
}

TSharedPtr<SButton> USH_WaterfallTool2Statics::CreateStorePageButton(FString StorePageURL, FString Tooltip)
{
	return CreateURLButton(
		(StorePageURL.IsEmpty()) ? SH_WaterfallTool2Globals::StorePage : StorePageURL, FText::FromString("Store Page"),
		FText::FromString((Tooltip.IsEmpty()) ? "Open the Fab store page for this tool!" : Tooltip),
		SNew(SImage).Image(FAppStyle::Get().GetBrush("MainFrame.OpenMarketplace"))
	);
}
#endif

FReply USH_WaterfallTool2Statics::ButtonClicked_OpenURL(FString URL)
{
	FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);

	return FReply::Handled();
}

FAssetData USH_WaterfallTool2Statics::GetAssetByPath(FSoftObjectPath InputObjectPath)
{
	IAssetRegistry& AssetRegistry = GetAssetRegistry();
	return AssetRegistry.GetAssetByObjectPath(InputObjectPath);
}

IAssetRegistry& USH_WaterfallTool2Statics::GetAssetRegistry()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	return AssetRegistry;
}

#if WITH_EDITOR
TSharedPtr<IPlugin> USH_WaterfallTool2Statics::GetThisPlugin()
{
	IPluginManager& PluginManager = IPluginManager::Get();
	return PluginManager.FindPlugin(SH_WaterfallTool2Globals::ThisPluginName);
}

FString USH_WaterfallTool2Statics::GetPluginVersion()
{
	TSharedPtr<IPlugin> ThisPlugin = GetThisPlugin();
	if (ThisPlugin.IsValid())
	{
		return ThisPlugin->GetDescriptor().VersionName;
	}

	return "";
}

FString USH_WaterfallTool2Statics::GetPluginDocsLink()
{
	TSharedPtr<IPlugin> ThisPlugin = GetThisPlugin();
	if (ThisPlugin.IsValid())
	{
		return ThisPlugin->GetDescriptor().DocsURL;
	}

	return "";
}

void USH_WaterfallTool2Statics::SelectActorInViewport(AActor* ActorToSelect, bool bFocusActor)
{
	if (GEditor)
	{
		UEditorActorSubsystem* ActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		if (ActorSubsystem)
		{
			ActorSubsystem->SetSelectedLevelActors({ ActorToSelect });

			if (bFocusActor && !GCurrentLevelEditingViewportClient->IsAnyActorLocked()) //Don't focus if the user is piloting an actor
			{
				//Focus the actor (this is what gets called when pressing 'F' in the viewport)
				if (GUnrealEd)
				{
					const FScopedTransaction Transaction(FText::FromString("Focus Waterfall"));

					GUnrealEd->Exec(ActorToSelect->GetWorld(), TEXT("CAMERA ALIGN ACTIVEVIEWPORTONLY"));
				}
			}
		}
	}
}
#endif

FVector USH_WaterfallTool2Statics::SlerpNormals(FVector NormalA, FVector NormalB, float Alpha)
{
	return FVector::SlerpNormals(NormalA, NormalB, Alpha);
}

#if WITH_EDITOR
FString USH_WaterfallTool2Statics::SummonFilePicker_ContentBrowser(FString DialogTitle, FString DefaultAssetName, FString DefaultAssetPath, TArray<FTopLevelAssetPath> AssetClasses)
{
	FSaveAssetDialogConfig SaveAssetDialogConfig;
	SaveAssetDialogConfig.DialogTitleOverride = FText::FromString(DialogTitle);
	SaveAssetDialogConfig.DefaultPath = DefaultAssetPath;
	SaveAssetDialogConfig.DefaultAssetName = DefaultAssetName;
	SaveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;
	SaveAssetDialogConfig.AssetClassNames = AssetClasses;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FString SaveObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);
	if (!SaveObjectPath.IsEmpty())
	{
		const FString SavePackagePath = FPaths::GetPath(SaveObjectPath);
		FEditorDirectories::Get().SetLastDirectory(ELastDirectory::NEW_ASSET, SavePackagePath);
	}

	return SaveObjectPath;
}
#endif

bool USH_WaterfallTool2Statics::ResampleSpline(class USplineComponent* InSpline, int32 NumPoints)
{
	if (InSpline && NumPoints >= 2)
	{
		float SegmentLength = 1.f / NumPoints;
		float WorkingTime = 0.f;
		TArray<FVector> SampledPoints = {};

		while (WorkingTime < 1.f)
		{
			SampledPoints.Add(InSpline->GetWorldLocationAtTime(WorkingTime));
			WorkingTime += SegmentLength;
		}
		SampledPoints.Add(InSpline->GetWorldLocationAtTime(1.f));

		InSpline->ClearSplinePoints();
		for (const FVector& SampledPoint : SampledPoints)
		{
			InSpline->AddSplinePoint(SampledPoint, ESplineCoordinateSpace::World, false);
		}

		InSpline->UpdateSpline();
		InSpline->GetComponentInstanceData()->ApplyToComponent(InSpline, ECacheApplyPhase::PostSimpleConstructionScript);
	}

	return false;
}