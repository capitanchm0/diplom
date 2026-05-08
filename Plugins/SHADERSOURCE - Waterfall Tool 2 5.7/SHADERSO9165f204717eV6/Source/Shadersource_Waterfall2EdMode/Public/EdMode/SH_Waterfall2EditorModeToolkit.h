// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/BaseToolkit.h"
#include "EditorComponents/SH_WaterfallGenerationEnums.h"

/* The toolkit to go with the Waterfall Tool 2 Editor Mode.
* (UI is mostly handled here and in the Details Customisation.) */
class FSH_Waterfall2EditorModeToolkit : public FModeToolkit
{
public:
	FSH_Waterfall2EditorModeToolkit();

	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual TSharedPtr<SWidget> GetInlineContent() const override { return ToolkitWidget; }
	virtual void InvokeUI() override;

	TObjectPtr<UObject> GetDetailsFocus() { return DetailsFocus; }
	void UpdateDetailsFocus(TObjectPtr<UObject> NewSettingsObject);
	void SetTookitWidgetEnabled(bool bEnabled, FSimpleDelegate OptionalCancelDelegate = FSimpleDelegate());

	FReply ButtonClicked_ChangeState(ESH_W2_TabState NewState);
	FReply ButtonClicked_AdvancedTab();

	void SetCancelDelegate(FSimpleDelegate NewCancelAction);
	void ClearCancelDelegate();

private:
	TSharedPtr<SWidget> ToolkitWidget;
	TSharedPtr<SWidget> HeaderWidget;
	TSharedPtr<SBorder> ActionButtonHolder;

	TSharedPtr<IDetailsView> DetailsPanel;
	TObjectPtr<UObject> DetailsFocus;

	//Whether to auto focus the waterfall when it's selected via the UI
	TSharedPtr<SCheckBox> CHB_AutoFocusWaterfall;

	//Shadersource Modes Toolbar
	FButtonStyle ButtonStyle_NoPadding = FButtonStyle();
	void ConstructShadersourceModesToolbarButtonStyle();
	TSharedPtr<SWidget> GenerateEdModeButton(FEditorModeInfo EdModeInfo);
	TSharedPtr<SWidget> GenerateShadersourceModesToolbar(TArray<FString> Filters = { "SHADERSOURCE" });

	TSharedPtr<FUICommandList> Commands;
	TSharedRef<SWidget> GenerateWaterfallsContextMenu();
	FText GetWaterfallSelectionText() const;
	const FText NoWaterfallSelected = FText::FromString("Select a Waterfall");
	bool bFocusedWaterfall = false;

	TArray<ESH_W2_TabState> AdvancedTabs =
	{
		ESH_W2_TabState::W2_TS_Paths,
		ESH_W2_TabState::W2_TS_Meshes,
		ESH_W2_TabState::W2_TS_Bake,
		ESH_W2_TabState::W2_TS_Materials,
		ESH_W2_TabState::W2_TS_FX
	};

	bool IsState(ESH_W2_TabState ThisState) const;
	bool IsNotState(ESH_W2_TabState ThisState) const;
	bool IsNotStates(TArray<ESH_W2_TabState> TheseStates) const;

	EVisibility ShowTabs() const;
	EVisibility ShowTabs_Advanced() const;
	EVisibility ShowLoadingBar() const;
	EVisibility ShowWaterfallSelection() const;

	FReply ButtonClicked_Cancel();

	FSimpleDelegate Del_Cancel = FSimpleDelegate();

	void RefreshDetailsPanel();

	bool bMainWidgetEnabled = true;
	TSharedPtr<SVerticalBox> VB_MainWidget = nullptr;
	TSharedPtr<SVerticalBox> VB_HeaderWidget = nullptr;
};
