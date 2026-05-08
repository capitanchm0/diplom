// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/* Waterfall Tool 2 Editor Mode module. */
class FShadersource_Waterfall2EdModeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	//Keyboard shortcuts, button commands, and menus
protected:
	TSharedPtr<class FUICommandList> PluginCommands;

	void RegisterSHADERSOURCEMenu();

private:
	TSharedRef<SWidget> FillComboButton(TSharedPtr<class FUICommandList> Commands);
	void CreateSubmenu(UToolMenu* InToolMenu);

	TArray<FName> MenuSectionNames;

protected:
	virtual void RegisterMenus();

	//Detail Customisation
protected:
	/** List of registered class that we must unregister when the module shuts down */
	TSet<FName> RegisteredClassNames;

	virtual void RegisterDetailsCustomisations();
	void RegisterCustomClassLayout(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate);
	void UnRegisterDetailsCustomisations();

	virtual void OnPostEngineInit();
	virtual void OnPreEngineExit();
};

/* Waterfall Tool 2 Editor Mode commands. */
class FSH_WaterfallTool2Commands : public TCommands<FSH_WaterfallTool2Commands>
{
public:

	FSH_WaterfallTool2Commands()
		: TCommands<FSH_WaterfallTool2Commands>(TEXT("SH_WaterfallTool2Commands"), NSLOCTEXT("Contexts", "SH_WaterfallTool2Commands", "SHADERSOURCE: Waterfall Tool 2 Plugin"), NAME_None, FAppStyle::GetAppStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public: //Editor Toolbar Button Command

	static TSharedRef<FUICommandList> ActionList;

	TSharedPtr<FUICommandInfo> OpenEdMode_SH_WaterfallTool2;
};
