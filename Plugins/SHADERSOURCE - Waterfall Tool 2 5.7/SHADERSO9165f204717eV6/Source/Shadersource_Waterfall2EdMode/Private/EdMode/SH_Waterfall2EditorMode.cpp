// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#include "EdMode/SH_Waterfall2EditorMode.h"
#include "EdMode/SH_Waterfall2EditorModeToolkit.h"
#include "EdMode/SH_Settings_WaterfallSelection.h"
#include "EdMode/SH_Waterfall2EditorModeStyle.h"
#include "Tools/EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Actors/SH_Waterfall2.h"
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "Styling/SlateIconFinder.h"

#define LOCTEXT_NAMESPACE "SH_Waterfall2EditorMode"

const FEditorModeID USH_Waterfall2EditorMode::EM_SH_Waterfall2EditorModeId = TEXT("EM_SHADERSOURCE_Waterfall2EditorMode");

USH_Waterfall2EditorMode::USH_Waterfall2EditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	//Make sure to initialize and reload the editor mode style before accessing the icon
	FSH_Waterfall2EditorModeStyle::Initialize();
	FSH_Waterfall2EditorModeStyle::ReloadTextures();

	FSlateIcon WFT2_Icon = FSlateIconFinder::FindIcon("SHADERSOURCE.WaterfallTool2_Single");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(USH_Waterfall2EditorMode::EM_SH_Waterfall2EditorModeId,
		LOCTEXT("ModeName", "SHADERSOURCE: Waterfall Tool 2"),
		WFT2_Icon,
		true);

	//Set the UObject Class to display in the toolkit
	SettingsClass = USH_Settings_WaterfallSelection::StaticClass();
}

USH_Waterfall2EditorMode::~USH_Waterfall2EditorMode()
{

}

void USH_Waterfall2EditorMode::Enter()
{
	UEdMode::Enter();

	//Set the visibility of all the static and dynamic meshes
	SetAllMeshVisibility(false);

	//Make sure to display the waterfall that's selected if there is one selected
	ActorSelectionChangeNotify();
}

void USH_Waterfall2EditorMode::Exit()
{	
	//Set the visibility of all the static and dynamic meshes
	SetAllMeshVisibility();
	HideAllKillPlanes();
	//Verify all the Fx components are visible
	SetFxVisibility(true);

	UEdMode::Exit();
}

bool USH_Waterfall2EditorMode::IsCompatibleWith(FEditorModeID OtherModeID) const
{
	return false;
}

void USH_Waterfall2EditorMode::ActorSelectionChangeNotify()
{
	if (GEditor)
	{
		USelection* CurrentSelection = GEditor->GetSelectedActors();
		if (TSharedPtr<FSH_Waterfall2EditorModeToolkit> WaterfallToolkit = StaticCastSharedPtr<FSH_Waterfall2EditorModeToolkit>(Toolkit))
		{
			HideAllKillPlanes();

			if (CurrentSelection)
			{
				AActor* MostRecentSelectedActor = CurrentSelection->GetTop<AActor>();
				if (ASH_Waterfall2* SelectedWaterfall = Cast<ASH_Waterfall2>(MostRecentSelectedActor))
				{
					//Cheat to refresh the slate
					WaterfallToolkit->UpdateDetailsFocus(nullptr);
					WaterfallToolkit->UpdateDetailsFocus(SelectedWaterfall->GetWaterfallSettings());
					SelectedWaterfall->GetKillPlane()->SetVisibility(true);
					SelectedWaterfall->OnEnableUI.BindRaw(WaterfallToolkit.Get(), &FSH_Waterfall2EditorModeToolkit::SetTookitWidgetEnabled);
					return;
				}
			}

			//If no valid waterfall asset is selected, go back to the settings object
			WaterfallToolkit->UpdateDetailsFocus(SettingsObject);
		}
	}
}

void USH_Waterfall2EditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FSH_Waterfall2EditorModeToolkit);

	if (TSharedPtr<FSH_Waterfall2EditorModeToolkit> WaterfallToolkit = StaticCastSharedPtr<FSH_Waterfall2EditorModeToolkit>(Toolkit))
	{
		WaterfallToolkit->UpdateDetailsFocus(SettingsObject);
	}
}

void USH_Waterfall2EditorMode::SetAllMeshVisibility()
{
	TArray<AActor*> FoundActors = {};
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASH_Waterfall2::StaticClass(), FoundActors);
	for (AActor* FoundActor : FoundActors)
	{
		if (ASH_Waterfall2* WaterfallActor = Cast<ASH_Waterfall2>(FoundActor))
		{
			WaterfallActor->SetMeshesVisibility(WaterfallActor->bShowDynamicMeshesInGame);
		}
	}
}

void USH_Waterfall2EditorMode::SetAllMeshVisibility(bool bDynamicMeshHidden)
{
	TArray<AActor*> FoundActors = {};
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASH_Waterfall2::StaticClass(), FoundActors);
	for (AActor* FoundActor : FoundActors)
	{
		if (ASH_Waterfall2* WaterfallActor = Cast<ASH_Waterfall2>(FoundActor))
		{
			WaterfallActor->SetMeshesVisibility(!bDynamicMeshHidden);
		}
	}
}

void USH_Waterfall2EditorMode::HideAllKillPlanes()
{
	TArray<AActor*> WaterfallActors = {};
	UGameplayStatics::GetAllActorsOfClass(this, ASH_Waterfall2::StaticClass(), WaterfallActors);
	for (AActor* WaterfallActor : WaterfallActors)
	{
		if (ASH_Waterfall2* Waterfall = Cast<ASH_Waterfall2>(WaterfallActor)) Waterfall->GetKillPlane()->SetVisibility(false);
	}
}

void USH_Waterfall2EditorMode::SetFxVisibility(bool bShowFx)
{
	TArray<AActor*> FoundActors = {};
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASH_Waterfall2::StaticClass(), FoundActors);
	for (AActor* FoundActor : FoundActors)
	{
		if (ASH_Waterfall2* WaterfallActor = Cast<ASH_Waterfall2>(FoundActor))
		{
			WaterfallActor->SetFxVisibility(bShowFx);
		}
	}
}

#undef LOCTEXT_NAMESPACE