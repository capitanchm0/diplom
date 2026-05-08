// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Details/SH_PathCompVisualiser.h"
#include "EditorComponents/SH_WaterfallPathComponent.h"
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "Slate/SSH_FxPointDisplay.h"
#include "Actors/SH_Waterfall2.h"

void FSH_PathCompVisualiser::GenerateContextMenuSections(FMenuBuilder& InMenuBuilder) const
{
	InMenuBuilder.BeginSection("WaterfallFx", FText::FromString("Waterfall Fx"));
	{
		USH_WaterfallPathComponent* Path = Cast<USH_WaterfallPathComponent>(GetEditedSplineComponent());

		InMenuBuilder.AddWidget(
			SNew(SSH_FxPointDisplay)
			.WindowSize(FVector2D(400.f,500.f))
			.PathIndex(Path->PathIndex)
			.PointIndex(SelectionState->GetLastKeyIndexSelected())
			.ParentWaterfall(Path->GetParentWaterfall())
			,
			FText::GetEmpty()
		);
	}
	InMenuBuilder.EndSection();

	FSplineComponentVisualizer::GenerateContextMenuSections(InMenuBuilder);
}

void FSH_PathCompVisualiser::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FSplineComponentVisualizer::DrawVisualization(Component, View, PDI);
}

void FSH_PathCompVisualiser::SelectSplinePointInViewport(USH_WaterfallPathComponent* InPathComponent, int32 PointIndex, bool bIsCtrlHeld)
{
	check(InPathComponent);

	if (GetEditedSplineComponent() != InPathComponent)
	{
		SetEditedSplineComponent(InPathComponent);
	}

	SelectSplinePoint(PointIndex, bIsCtrlHeld);
}

void FSH_PathCompVisualiser::ChangeSelectionState(int32 Index, bool bIsCtrlHeld)
{
	FSplineComponentVisualizer::ChangeSelectionState(Index, bIsCtrlHeld);

	//Update UI for Fx Tab
	if (USH_WaterfallPathComponent* Path = Cast<USH_WaterfallPathComponent>(GetEditedSplineComponent()))
	{
		Path->LastSelectedSplinePoint = SelectionState->GetLastKeyIndexSelected();

		//Set all the other paths to INDEX_NONE
		if (Path->GetParentWaterfall())
		{
			for (USH_WaterfallPathComponent* OtherPath : Path->GetParentWaterfall()->GetAllPathComponents())
			{
				if (OtherPath != Path)
				{
					OtherPath->LastSelectedSplinePoint = INDEX_NONE;
				}
			}
		}
	}
}

void FSH_PathCompVisualiser::TrackingStopped(FEditorViewportClient* InViewportClient, bool bInDidMove)
{
	FSplineComponentVisualizer::TrackingStopped(InViewportClient, bInDidMove);
}