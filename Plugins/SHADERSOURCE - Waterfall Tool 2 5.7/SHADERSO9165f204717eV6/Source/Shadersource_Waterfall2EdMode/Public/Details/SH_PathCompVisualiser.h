// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SplineComponentVisualizer.h"

/* Custom spline visualiser to add custom functionality to spline point display. */
class SHADERSOURCE_WATERFALL2EDMODE_API FSH_PathCompVisualiser : public FSplineComponentVisualizer
{
public:
	/*FSplineComponentVisualizer~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/** Add menu sections to the context menu */
	virtual void GenerateContextMenuSections(FMenuBuilder& InMenuBuilder) const;
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/** Select spline point at specified index - public wrapper*/
	virtual void SelectSplinePointInViewport(class USH_WaterfallPathComponent* InPathComponent, int32 PointIndex, bool bIsCtrlHeld = false);

protected:
	virtual void ChangeSelectionState(int32 Index, bool bIsCtrlHeld) override;
	virtual void TrackingStopped(FEditorViewportClient* InViewportClient, bool bInDidMove) override;
};
