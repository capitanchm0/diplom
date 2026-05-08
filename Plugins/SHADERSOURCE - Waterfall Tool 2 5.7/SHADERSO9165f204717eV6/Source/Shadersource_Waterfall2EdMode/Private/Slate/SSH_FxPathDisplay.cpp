// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Slate/SSH_FxPathDisplay.h"
#include "Slate/SSH_FxPointDisplay.h"
#include "Details/SH_PathCompVisualiser.h"
#include "Details/SH_Details_WaterfallEdit.h"
#include "EditorComponents/SH_WaterfallPathComponent.h"
#include "Components/SH_WaterfallVFXComponent.h"
#include "Components/SH_WaterfallSFXComponent.h"
#include "Actors/SH_Waterfall2.h"
#include "LevelEditor.h"
#include "SAssetDropTarget.h"
#include "NiagaraSystem.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "LevelEditorViewport.h"
#include "Sound/SoundBase.h"

void SSH_FxPathDisplay::Construct(const FArguments& InArgs)
{
	ParentWaterfall = InArgs._ParentWaterfall.Get();
	PathIndex = InArgs._PathIndex.Get();

	TSharedPtr<SVerticalBox> VB_Points = SNew(SVerticalBox);
	for (int i = 0; i < ParentWaterfall->GetPathComponent(PathIndex)->GetNumberOfSplinePoints(); i++)
	{
		VB_Points->AddSlot()
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.AutoHeight()
			.Padding(0.f, 5.f)
			[
				SNew(SAssetDropTarget)
				.bSupportsMultiDrop(true)
				.OnAreAssetsAcceptableForDrop_Raw(this, &SSH_FxPathDisplay::AreAssetsValidForDrop)
				.OnAssetsDropped_Raw(this, &SSH_FxPathDisplay::OnFxAssetsDropped, i)
				[
					SNew(SButton)
					.OnClicked(this, &SSH_FxPathDisplay::ButtonClicked_Point, i)
					.ContentPadding(0.f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SImage)
							.ColorAndOpacity(this, &SSH_FxPathDisplay::GetColour_Point, i)
						]
						+ SOverlay::Slot()
						.HAlign(EHorizontalAlignment::HAlign_Center)
						.VAlign(EVerticalAlignment::VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::SanitizeFloat(i, 0)))
						]
					]
				]
			];
	}

	ChildSlot
		[
			VB_Points.ToSharedRef()
		];
};

void SSH_FxPathDisplay::SummonFxMenu(int32 _PointIndex)
{
	//Summon the menu popup to add, edit, and remove FX components
	const FVector2D WindowSize(400.f, 500.f);

	TSharedRef<SWidget> ActualWidget = SNew(SSH_FxPointDisplay)
		.WindowSize(WindowSize)
		.PathIndex(PathIndex)
		.PointIndex(_PointIndex)
		.ParentWaterfall(ParentWaterfall)
		;

	// Wrap the picker widget in a multibox-style menu body
	FMenuBuilder MenuBuilder(/*BShouldCloseAfterSelection=*/ false, /*CommandList=*/ nullptr);
	MenuBuilder.BeginSection("EditPointFX");
	MenuBuilder.AddWidget(ActualWidget, FText::GetEmpty(), /*bNoIndent=*/ true);
	MenuBuilder.EndSection();

	// Determine where the pop-up should open
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (!ParentWindow.IsValid())
	{
		TSharedPtr<SDockTab> LevelEditorTab = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>("LevelEditor").GetLevelEditorTab();
		ParentWindow = LevelEditorTab->GetParentWindow();
		check(ParentWindow.IsValid());
	}

	// Open the pop-up
	FPopupTransitionEffect TransitionEffect(FPopupTransitionEffect::None);
	TSharedPtr<IMenu> SummonedMenu = FSlateApplication::Get().PushMenu(ParentWindow.ToSharedRef(), FWidgetPath(), ActualWidget, LastRecordedMousePos, TransitionEffect, /*bFocusImmediately=*/ true);
	if (SummonedMenu.IsValid())
	{
		SummonedMenu->GetOnMenuDismissed().AddRaw(this, &SSH_FxPathDisplay::OnMenuDismissed);
	}
}

FReply SSH_FxPathDisplay::ButtonClicked_Point(int32 Index)
{
	LastRecordedMousePos = FSlateApplication::Get().GetCursorPos();

	//Clear the last selected spline point for all except this path and set this path to Index
	for (USH_WaterfallPathComponent* OtherPath : ParentWaterfall->GetAllPathComponents())
	{
		OtherPath->LastSelectedSplinePoint = (OtherPath->PathIndex == PathIndex) ? Index : INDEX_NONE;
	}

	//We've reselected the actor, so now we have to reselect the spline point
	USH_WaterfallPathComponent* Path = ParentWaterfall->GetPathComponent(PathIndex);
	if (Path)
	{
		TSharedPtr<FComponentVisualizer> Visualizer = GUnrealEd->FindComponentVisualizer(USH_WaterfallPathComponent::StaticClass());
		TSharedPtr<FSH_PathCompVisualiser> SplineVisualizer = StaticCastSharedPtr<FSH_PathCompVisualiser>(Visualizer);

		SplineVisualizer->SelectSplinePointInViewport(Path, Path->LastSelectedSplinePoint);

		GUnrealEd->ComponentVisManager.SetActiveComponentVis(GCurrentLevelEditingViewportClient, Visualizer);
	}

	SummonFxMenu(Index);

	return FReply::Handled();
}

FSlateColor SSH_FxPathDisplay::GetColour_Point(int32 Index) const
{
	USH_WaterfallPathComponent* Path = ParentWaterfall->GetPathComponent(PathIndex);
	if (Path)
	{
		if (Path->GetLastSelectedSplinePoint() == Index) return FColor::Yellow;
		else if (Path->DoesSimulatedPointHaveFX(Index)) return FColor::Green;
	}

	return FColor::Red;
}

void SSH_FxPathDisplay::OnMenuDismissed(TSharedRef<IMenu> DismissedMenu)
{
	ParentWaterfall->RefreshDetailsPanel();
}

bool SSH_FxPathDisplay::AreAssetsValidForDrop(TArrayView<FAssetData> DroppingAssets)
{
	bool bContainsNiagraSystem = false;
	FTopLevelAssetPath NiagaraSystemClassPath = UNiagaraSystem::StaticClass()->GetClassPathName();
	for (FAssetData DroppingAsset : DroppingAssets)
	{
		if (DroppingAsset.AssetClassPath == NiagaraSystemClassPath)
		{
			bContainsNiagraSystem = true;
			break;
		}
	}
	
	return bContainsNiagraSystem;
}

void SSH_FxPathDisplay::OnFxAssetsDropped(const FDragDropEvent& DragDropEvent, TArrayView<FAssetData> DroppingAssets, int32 PointIndex)
{
	const FScopedTransaction Transaction(FText::FromString("Drop FX"));

	//Add a new FxComponent for each dropped Niagara System
	FTopLevelAssetPath NiagaraSystemClassPath = UNiagaraSystem::StaticClass()->GetClassPathName();
	for (FAssetData DroppingAsset : DroppingAssets)
	{
		if (DroppingAsset.AssetClassPath == NiagaraSystemClassPath)
		{
			USH_WaterfallVFXComponent* NewFxComponent = Cast<USH_WaterfallVFXComponent>(SSH_FxPointDisplay::AddComponent_FX(ParentWaterfall, PathIndex, PointIndex, ESH_W2_FxType::VFX));
			if (NewFxComponent)
			{
				NewFxComponent->SetAsset(Cast<UNiagaraSystem>(DroppingAsset.GetAsset()));
			}
		}
		else if (DroppingAsset.GetClass()->IsChildOf(USoundBase::StaticClass()))
		{
			USH_WaterfallSFXComponent* NewFxComponent = Cast<USH_WaterfallSFXComponent>(SSH_FxPointDisplay::AddComponent_FX(ParentWaterfall, PathIndex, PointIndex, ESH_W2_FxType::SFX));
			if (NewFxComponent)
			{
				NewFxComponent->SetSound(Cast<USoundBase>(DroppingAsset.GetAsset()));
			}
		}
	}
}