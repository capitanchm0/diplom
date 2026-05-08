// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
class ASH_Waterfall2;

/* Widget to represent an individual path in the Fx Tab of the Ed Mode. */
class SHADERSOURCE_WATERFALL2EDMODE_API SSH_FxPathDisplay : public SCompoundWidget
{
	//friend class SSH_PointFX;
public:
	SLATE_BEGIN_ARGS(SSH_FxPathDisplay) {}
		SLATE_ATTRIBUTE(ASH_Waterfall2*, ParentWaterfall)
		SLATE_ATTRIBUTE(int32, PathIndex)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

private:
	ASH_Waterfall2* ParentWaterfall = nullptr;
	int32 PathIndex = 0;
	FVector2D LastRecordedMousePos = FVector2D::ZeroVector;

	void SummonFxMenu(int32 _PointIndex);

	FReply ButtonClicked_Point(int32 Index);

	FSlateColor GetColour_Point(int32 Index) const;

	void OnMenuDismissed(TSharedRef<IMenu> DismissedMenu);

	bool AreAssetsValidForDrop(TArrayView<FAssetData> DroppingAssets);
	void OnFxAssetsDropped(const FDragDropEvent& DragDropEvent, TArrayView<FAssetData> DroppingAssets, int32 PointIndex);
};
