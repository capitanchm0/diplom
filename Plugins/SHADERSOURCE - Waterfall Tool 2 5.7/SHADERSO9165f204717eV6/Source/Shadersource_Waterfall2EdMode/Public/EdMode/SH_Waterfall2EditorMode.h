// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
/*Using UEdModeDefault and UsesPropertyWidgets enables the transform gizmos and associated buttons*/
#include "Tools/DefaultEdMode.h"
#include "SH_Waterfall2EditorMode.generated.h"

/* Waterfall Tool 2 Editor Mode */
UCLASS()
class USH_Waterfall2EditorMode : public UEdModeDefault
{
	GENERATED_BODY()

public:
	const static FEditorModeID EM_SH_Waterfall2EditorModeId;

	USH_Waterfall2EditorMode();
	virtual ~USH_Waterfall2EditorMode();

	/** UEdMode interface */
	virtual void Enter() override;
	virtual void Exit() override;
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override;
	/** Check to see if an actor can be selected in this mode - no side effects */
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelection) const override { return true; }
	virtual void ActorSelectionChangeNotify() override;
	virtual void CreateToolkit() override;
	virtual bool UsesPropertyWidgets() const override { return true; }

private:
	/*This version sets the visibility to the bool on the Waterfall Actor*/
	void SetAllMeshVisibility();
	/*This version ignores the visibility bool on the Waterfall Actor*/
	void SetAllMeshVisibility(bool bDynamicMeshHidden);
	void HideAllKillPlanes();
	void SetFxVisibility(bool bShowFx);
};
