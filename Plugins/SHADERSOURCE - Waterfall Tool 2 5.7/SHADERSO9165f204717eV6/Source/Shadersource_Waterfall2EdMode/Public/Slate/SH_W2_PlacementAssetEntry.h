// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/*A custom slate that is based on the default editor private class [SPlacementAssetEntry]*/
class SHADERSOURCE_WATERFALL2EDMODE_API SSH_W2_PlacementAssetEntry : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSH_W2_PlacementAssetEntry) {}
		SLATE_ATTRIBUTE(FAssetData, TargetAsset)
		SLATE_ATTRIBUTE(TSharedPtr<FAssetThumbnailPool>, ThumbnailPool)
		SLATE_ATTRIBUTE(FText, AssetTitle)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

protected:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	FAssetData TargetAssetData;
	bool bIsPressed;
	TSharedPtr<FAssetThumbnail> Thumbnail;

	/** Brush resource that represents a button */
	const FSlateBrush* NormalImage;
	/** Brush resource that represents a button when it is hovered */
	const FSlateBrush* HoverImage;
	/** Brush resource that represents a button when it is pressed */
	const FSlateBrush* PressedImage;

	const FSlateBrush* GetBorder() const;
};