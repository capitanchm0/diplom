// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "SH_WaterfallTool2Statics.generated.h"

/* Shadersource globals. */
namespace SH_WaterfallTool2Globals
{
	const FString ThisPluginName = "SHADERSOURCE_WaterfallTool2";
	const FString StorePage = "https://www.fab.com/listings/f44cd11c-4202-4e63-b74d-be09b1749b6e";

	const FName SHADERSOURCEMainMenuName = FName("SHADERSOURCEMainMenu");
}

/* Slate button styles. */
enum ESH_W2_ButtonStyle
{
	W2_BS_Default,
	W2_BS_Primary,
	W2_BS_Success,
	W2_BS_Info,
	W2_BS_Warning,
	W2_BS_Danger,
};

/* Editor notification state. */
enum class ESH_W2_NotificationState : uint8
{
	W2_NS_None,
	W2_NS_Pending,
	W2_NS_Success,
	W2_NS_Fail,
};

/* Static functions for use across the Waterfall Tool 2 plugin. */
UCLASS()
class SHADERSOURCE_WATERFALL2RUNTIME_API USH_WaterfallTool2Statics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	//Common Tools
public:
#if WITH_EDITOR
	/*Sends a message to pop up in the corner of the editor, like "Compile Complete", etc.*/
	static TSharedPtr<SNotificationItem> PrintEditorNotification(FText inText, ESH_W2_NotificationState State = ESH_W2_NotificationState::W2_NS_Success, bool bUseSuccessFailIcons = true, bool bFireAndForget = true, float FadeInDuration = 0.1f, float FadeOutDuration = 0.5f, float ExpireDuration = 5.f, bool bUseLargeFont = true, bool bUseThrobber = false, bool bAllowThrottleWhenFrameRateIsLow = false);
	static TSharedPtr<SNotificationItem> PrintEditorNotification(FString inString, ESH_W2_NotificationState State = ESH_W2_NotificationState::W2_NS_Success, bool bUseSuccessFailIcons = true, bool bFireAndForget = true, float FadeInDuration = 0.1f, float FadeOutDuration = 0.5f, float ExpireDuration = 5.f, bool bUseLargeFont = true, bool bUseThrobber = false, bool bAllowThrottleWhenFrameRateIsLow = false)
	{
		return PrintEditorNotification(FText::FromString(inString), State, bUseSuccessFailIcons, bFireAndForget, FadeInDuration, FadeOutDuration, ExpireDuration, bUseLargeFont, bUseThrobber, bAllowThrottleWhenFrameRateIsLow);
	}
#endif

	/*Quick easy way to print a message to the general log in C++.*/
	static void PrintMessageToLog(FString message, ELogVerbosity::Type Verbosity = ELogVerbosity::Log);
	static void PrintMessageToLog(FText message, ELogVerbosity::Type Verbosity = ELogVerbosity::Log) { PrintMessageToLog(message.ToString(), Verbosity); }

	//Editor Tools
public:
#if WITH_EDITOR
	static void InvokeEditorMode(FEditorModeID EdModeID);
	static FReply ButtonClicked_InvokeEditorMode(FEditorModeID EdModeID);
#endif

	//Slate Statics
public:
	static TSharedPtr<SButton> CreateButton(FString Text, FOnClicked OnClicked, ESH_W2_ButtonStyle Style = ESH_W2_ButtonStyle::W2_BS_Default, bool bDisable = false, FString Tooltip = "");
#if WITH_EDITOR
	static TSharedPtr<SButton> AddBasicButtonRow(class IDetailCategoryBuilder& InCategory, FString Text, FOnClicked OnClicked, ESH_W2_ButtonStyle Style = ESH_W2_ButtonStyle::W2_BS_Default, bool bDisable = false, FString Tooltip = "", bool bValueContentOnly = false);

	static TSharedPtr<IDetailsView> ConstructDetailsPanel(UObject* SettingsObject, bool bAllowSearch = true, bool bShowObjectLabel = false);
	static TSharedPtr<IDetailsView> ConstructDetailsPanel(TArray<UObject*> SettingsObjects, bool bAllowSearch = true, bool bShowObjectLabel = false);
	static TSharedPtr<SWidget> ConstructBasicDetailsPanelWidget(TSharedPtr<IDetailsView> InDetailsPanel, FString DocumentationURL = "", FString StorePageURL = "", TSharedPtr<SWidget> AdditionalHeaderContent = TSharedPtr<SWidget>());
#endif

	static TSharedPtr<SButton> CreateURLButton(FString URL, FText ButtonText = FText::GetEmpty(), FText TooltipText = FText::GetEmpty(), TSharedPtr<SImage> ButtonImage = TSharedPtr<SImage>());
#if WITH_EDITOR
	static TSharedPtr<SButton> CreateDocumentationButton(FString DocumentationURL = "", FString Tooltip = "");
	static TSharedPtr<SButton> CreateStorePageButton(FString StorePageURL = "", FString Tooltip = "");
#endif

private:
	static FReply ButtonClicked_OpenURL(FString URL);

	//Asset Statics
public:
	/*Native helper function to get a single asset by path*/
	static FAssetData GetAssetByPath(FSoftObjectPath InputObjectPath);
	/*Native helper function to get the asset registry
	* Make this equal a reference (IAssetRegistry& AssetRegistry = GetAssetRegistry())
	* Don't forget: #include "AssetRegistry/AssetRegistryModule.h"*/
	static class IAssetRegistry& GetAssetRegistry();

	//Plugin Tools
public:
#if WITH_EDITOR
	static TSharedPtr<class IPlugin> GetThisPlugin();
	static FString GetPluginVersion();
	static FString GetPluginDocsLink();
#endif

	//Waterfall Tool 2 Statics
public:
#if WITH_EDITOR
	static void SelectActorInViewport(AActor* ActorToSelect, bool bFocusActor);
#endif

	/* Wrapper function - compiler errors if just use native function outright, possibly to do with nested Slerps */
	static FVector SlerpNormals(FVector NormalA, FVector NormalB, float Alpha);

#if WITH_EDITOR
	//Returns an object path (relative to '/Game/')
	static FString SummonFilePicker_ContentBrowser(FString DialogTitle, FString DefaultAssetName, FString DefaultAssetPath = "/Game/", TArray<FTopLevelAssetPath> AssetClasses = {});
#endif

	static bool ResampleSpline(class USplineComponent* InSpline, int32 NumPoints);
};
