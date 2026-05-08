// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#include "Shadersource_Waterfall2EdModeModule.h"
#include "EdMode/SH_Waterfall2EditorMode.h"
#include "Styling/SlateIconFinder.h"
#include "SH_WaterfallTool2Statics.h"
#include "EdMode/SH_Waterfall2EditorModeStyle.h"
#include "Interfaces/IMainFrameModule.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "EditorComponents/SH_WaterfallPathComponent.h"
#include "EditorModeRegistry.h"

#include "Details/SH_Details_Waterfall2.h"
#include "Details/SH_Details_WaterfallEdit.h"
#include "Details/SH_Details_WaterfallSelection.h"
#include "Details/SH_Details_WaterfallMeshComp.h"
#include "Details/SH_PathCompVisualiser.h"

TSharedRef<FUICommandList> FSH_WaterfallTool2Commands::ActionList(new FUICommandList());

#define LOCTEXT_NAMESPACE "Shadersource_Waterfall2EdModeModule"

void FShadersource_Waterfall2EdModeModule::StartupModule()
{
	FSH_Waterfall2EditorModeStyle::Initialize();
	FSH_Waterfall2EditorModeStyle::ReloadTextures();

	FSH_WaterfallTool2Commands::Register();

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FShadersource_Waterfall2EdModeModule::RegisterMenus));

	RegisterDetailsCustomisations();

	FCoreDelegates::OnEnginePreExit.AddRaw(this, &FShadersource_Waterfall2EdModeModule::OnPreEngineExit);
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FShadersource_Waterfall2EdModeModule::OnPostEngineInit);

	

	//Note: With UEdMode, the mode gets automatically registered here: UAssetEditorSubsystem::RegisterEditorModes so they don't need to be registered like the legacy FEdModes
}

void FShadersource_Waterfall2EdModeModule::ShutdownModule()
{
	FSH_WaterfallTool2Commands::Unregister();

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	UnRegisterDetailsCustomisations();

	FSH_Waterfall2EditorModeStyle::Shutdown();
}

void FShadersource_Waterfall2EdModeModule::RegisterSHADERSOURCEMenu()
{
	if (!UToolMenus::Get()->IsMenuRegistered(SH_WaterfallTool2Globals::SHADERSOURCEMainMenuName))
	{
		UToolMenu* MainSHADERSOURCEMenu = UToolMenus::Get()->RegisterMenu(SH_WaterfallTool2Globals::SHADERSOURCEMainMenuName);
		MainSHADERSOURCEMenu->Context = FToolMenuContext(PluginCommands);

		TAttribute<FSlateIcon> ButtonIcon_SHADERSOURCE = FSlateIconFinder::FindIcon("SHADERSOURCE.SHADERSOURCEIcon");

		{
			UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.ModesToolBar");
			{
				FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("SHADERSOURCE");
				{
					FUIAction TempCompileOptionsCommand;
					FToolMenuEntry& Entry_SHADERSOURCE = Section.AddEntry(FToolMenuEntry::InitComboButton("SHADERSOURCEButton", TempCompileOptionsCommand, FOnGetContent::CreateRaw(this, &FShadersource_Waterfall2EdModeModule::FillComboButton, PluginCommands), FText::FromString("SHADERSOURCE"), FText::FromString("Tools by SHADERSOURCE!"), ButtonIcon_SHADERSOURCE));
				}
			}
		}

		{
			UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
			{
				FToolMenuSection& Section = Menu->FindOrAddSection("SHADERSOURCE");
				Section.AddEntry(FToolMenuEntry::InitSubMenu(FName("SHADERSOURCESubmenu"), FText::FromString("SHADERSOURCE"), FText::FromString("Tools by SHADERSOURCE!"), FNewToolMenuDelegate::CreateRaw(this, &FShadersource_Waterfall2EdModeModule::CreateSubmenu), false, ButtonIcon_SHADERSOURCE));
			}
		}
	}
}

TSharedRef<SWidget> FShadersource_Waterfall2EdModeModule::FillComboButton(TSharedPtr<FUICommandList> Commands)
{
	UToolMenu* MainSHADERSOURCEMenu = UToolMenus::Get()->FindMenu(SH_WaterfallTool2Globals::SHADERSOURCEMainMenuName);

	if (MainSHADERSOURCEMenu)
	{
		return UToolMenus::Get()->GenerateWidget(MainSHADERSOURCEMenu);
	}

	return SNew(STextBlock)
		.Text(FText::FromString("\"MainSHADERSOURCEMenu\" not valid! (FShadersource_Waterfall2EdModeModule::FillComboButton)"));
}

void FShadersource_Waterfall2EdModeModule::CreateSubmenu(UToolMenu* InToolMenu)
{
	UToolMenu* MainSHADERSOURCEMenu = UToolMenus::Get()->FindMenu(SH_WaterfallTool2Globals::SHADERSOURCEMainMenuName);

	if (MainSHADERSOURCEMenu)
	{
		InToolMenu->Context = FToolMenuContext(PluginCommands);
		for (FToolMenuSection Section : MainSHADERSOURCEMenu->Sections)
		{
			FToolMenuSection& NewSection = InToolMenu->FindOrAddSection(Section.Name);
			NewSection.Label = FText::FromName(Section.Name);
			for (FToolMenuEntry Entry : Section.Blocks)
			{
				FToolMenuEntry& NewEntry = NewSection.AddEntry(Entry);
				if (MenuSectionNames.Contains(Section.Name)) //Then it's from this list
				{
					NewEntry.SetCommandList(PluginCommands);
				}
			}
		}
	}
	else
	{
		USH_WaterfallTool2Statics::PrintMessageToLog("Unable to get MainSHADERSOURCEMenu - FShadersource_Waterfall2EdModeModule::CreateSubmenu");
	}
}

void FShadersource_Waterfall2EdModeModule::RegisterMenus()
{
	RegisterSHADERSOURCEMenu();
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(SH_WaterfallTool2Globals::SHADERSOURCEMainMenuName);
		{
			FName SectionName_ShadersourceModes = FName("SHADERSOURCE Modes");
			FToolMenuSection& Section_SHADERSOURCE = Menu->FindOrAddSection(SectionName_ShadersourceModes);
			Section_SHADERSOURCE.Label = FText::FromName(SectionName_ShadersourceModes);
			Section_SHADERSOURCE.AddMenuEntryWithCommandList(FSH_WaterfallTool2Commands::Get().OpenEdMode_SH_WaterfallTool2, FSH_WaterfallTool2Commands::Get().ActionList, TAttribute<FText>(), TAttribute<FText>(), FSlateIconFinder::FindIcon("SHADERSOURCE.WaterfallTool2_Single"));
		}
	}
}

void FShadersource_Waterfall2EdModeModule::RegisterDetailsCustomisations()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	RegisterCustomClassLayout("SH_Waterfall2", FOnGetDetailCustomizationInstance::CreateStatic(&SH_Details_Waterfall2::MakeInstance));
	RegisterCustomClassLayout("SH_WaterfallSettingsComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FSH_Details_WaterfallEdit::MakeInstance));
	RegisterCustomClassLayout("SH_Settings_WaterfallSelection", FOnGetDetailCustomizationInstance::CreateStatic(&FSH_Details_WaterfallSelection::MakeInstance));
	RegisterCustomClassLayout("SH_WaterfallMeshComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FSH_Details_WaterfallMeshComp::MakeInstance));
}

void FShadersource_Waterfall2EdModeModule::RegisterCustomClassLayout(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate)
{
	check(ClassName != NAME_None);

	RegisteredClassNames.Add(ClassName);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomClassLayout(ClassName, DetailLayoutDelegate);
}

void FShadersource_Waterfall2EdModeModule::UnRegisterDetailsCustomisations()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		// Unregister all classes customized by name
		for (auto It = RegisteredClassNames.CreateConstIterator(); It; ++It)
		{
			if (It->IsValid())
			{
				PropertyModule.UnregisterCustomClassLayout(*It);
			}
		}

		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FShadersource_Waterfall2EdModeModule::OnPostEngineInit()
{
	//This needs to be called PostEngineInit so that GUnrealEd is valid
	if (GUnrealEd) GUnrealEd->RegisterComponentVisualizer(USH_WaterfallPathComponent::StaticClass()->GetFName(), MakeShareable(new FSH_PathCompVisualiser));
}

void FShadersource_Waterfall2EdModeModule::OnPreEngineExit()
{
	//This needs to be called PreEngineExit so that GUnrealEd is definitely still valid
	if (GUnrealEd) GUnrealEd->UnregisterComponentVisualizer(USH_WaterfallPathComponent::StaticClass()->GetFName());
}

void FSH_WaterfallTool2Commands::RegisterCommands()
{
	//Editor Toolbar Button
	UI_COMMAND(OpenEdMode_SH_WaterfallTool2, "SHADERSOURCE: Waterfall Tool 2", "Open the SHADERSOURCE Waterfall Tool 2 Editor Mode", EUserInterfaceActionType::Button, FInputChord());
	ActionList->MapAction(OpenEdMode_SH_WaterfallTool2, FExecuteAction::CreateStatic(&USH_WaterfallTool2Statics::InvokeEditorMode, USH_Waterfall2EditorMode::EM_SH_Waterfall2EditorModeId), FCanExecuteAction());

	IMainFrameModule& MainFrame = FModuleManager::Get().LoadModuleChecked<IMainFrameModule>("MainFrame");
	MainFrame.GetMainFrameCommandBindings()->Append(ActionList);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FShadersource_Waterfall2EdModeModule, Shadersource_Waterfall2EdMode)