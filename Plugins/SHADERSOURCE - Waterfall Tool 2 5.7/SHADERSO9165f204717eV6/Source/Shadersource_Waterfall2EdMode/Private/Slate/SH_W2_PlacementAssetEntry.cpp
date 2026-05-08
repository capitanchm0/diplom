// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Slate/SH_W2_PlacementAssetEntry.h"
#include "DragAndDrop/AssetDragDropOp.h"

void SSH_W2_PlacementAssetEntry::Construct(const FArguments& InArgs)
{
	TargetAssetData = InArgs._TargetAsset.Get();

	//Use Manual set title if attribute set otherwise just use the asset name
	FText Title = (InArgs._AssetTitle.IsSet()) ? InArgs._AssetTitle.Get() : FText::FromName(TargetAssetData.AssetName);

	Thumbnail = MakeShareable(new FAssetThumbnail(TargetAssetData, /*Width = */32, /*Height = */32, InArgs._ThumbnailPool.Get()));

	const FButtonStyle& ButtonStyle = FAppStyle::GetWidgetStyle<FButtonStyle>("PlacementBrowser.Asset");

	NormalImage = &ButtonStyle.Normal;
	HoverImage = &ButtonStyle.Hovered;
	PressedImage = &ButtonStyle.Pressed;

	ChildSlot
		.Padding(FMargin(8.f, 2.f, 12.f, 2.f))
		[
			SNew(SOverlay)

				+ SOverlay::Slot()
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("PlacementBrowser.Asset.Background"))
						.Cursor(EMouseCursor::GrabHand)
						.Padding(0)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.Padding(8.0f, 4.f)
								.AutoWidth()
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SNew(SBox)
										.WidthOverride(40)
										.HeightOverride(40)
										[
											Thumbnail->MakeThumbnailWidget()
										]
								]
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Fill)
								.Padding(0)
								[
									SNew(SBorder)
										.BorderImage(FAppStyle::Get().GetBrush("PlacementBrowser.Asset.LabelBack"))
										[
											SNew(SHorizontalBox)
												+ SHorizontalBox::Slot()
												.Padding(9, 0, 0, 1)
												.VAlign(VAlign_Center)
												[
													SNew(STextBlock)
														.TextStyle(FAppStyle::Get(), "PlacementBrowser.Asset.Name")
														.Text(Title)
												]
										]
								]
						]
				]
				+ SOverlay::Slot()
				[
					SNew(SBorder)
						.BorderImage(this, &SSH_W2_PlacementAssetEntry::GetBorder)
						.Cursor(EMouseCursor::GrabHand)
				]
		];
}

FReply SSH_W2_PlacementAssetEntry::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPressed = true;

		return FReply::Handled().DetectDrag(SharedThis(this), MouseEvent.GetEffectingButton());
	}

	return FReply::Unhandled();
}

FReply SSH_W2_PlacementAssetEntry::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPressed = false;
	}

	return FReply::Unhandled();
}

FReply SSH_W2_PlacementAssetEntry::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	bIsPressed = false;

	if (FEditorDelegates::OnAssetDragStarted.IsBound())
	{
		FEditorDelegates::OnAssetDragStarted.Broadcast({ TargetAssetData }, nullptr);
		return FReply::Handled();
	}

	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(TargetAssetData, nullptr));
	}
	else
	{
		return FReply::Handled();
	}
}

const FSlateBrush* SSH_W2_PlacementAssetEntry::GetBorder() const
{
	if (bIsPressed)
	{
		return PressedImage;
	}
	else if (IsHovered())
	{
		return HoverImage;
	}
	else
	{
		return NormalImage;
	}
}