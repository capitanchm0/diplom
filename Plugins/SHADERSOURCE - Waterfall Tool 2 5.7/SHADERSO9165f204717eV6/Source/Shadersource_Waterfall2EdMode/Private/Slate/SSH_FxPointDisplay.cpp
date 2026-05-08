// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Slate/SSH_FxPointDisplay.h"
#include "EditorComponents/SH_WaterfallPathComponent.h"
#include "Components/SH_WaterfallVFXComponent.h"
#include "Components/SH_WaterfallSFXComponent.h"
#include "Actors/SH_Waterfall2.h"
#include "SH_WaterfallTool2Statics.h"

void SSH_FxPointDisplay::Construct(const FArguments& InArgs)
{
	ParentWaterfall = InArgs._ParentWaterfall.Get();
	PathIndex = InArgs._PathIndex.Get();
	PointIndex = InArgs._PointIndex.Get();

	//Rather than passing through the paths, pass through the parent waterfall isntead and get the path that matches the path index to avoid crashes
	USH_WaterfallPathComponent* Path = ParentWaterfall->GetPathComponent(PathIndex);
	bool bHasFX = (Path) ? Path->DoesSimulatedPointHaveFX(PointIndex) : false;

	UpdateTargetComponents();

	DetailsPanel = USH_WaterfallTool2Statics::ConstructDetailsPanel(nullptr, true, true);
	UpdateFocus();

	ChildSlot
		[
			SNew(SBox)
			.WidthOverride(InArgs._WindowSize.Get().X)
			.HeightOverride(InArgs._WindowSize.Get().Y)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Top)
				.Padding(5.f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						USH_WaterfallTool2Statics::CreateButton("Add VFX", FOnClicked::CreateRaw(this, &SSH_FxPointDisplay::ButtonClicked_AddFX, ESH_W2_FxType::VFX), ESH_W2_ButtonStyle::W2_BS_Success).ToSharedRef()
					]
					+ SHorizontalBox::Slot()
					[
						USH_WaterfallTool2Statics::CreateButton("Add SFX", FOnClicked::CreateRaw(this, &SSH_FxPointDisplay::ButtonClicked_AddFX, ESH_W2_FxType::SFX), ESH_W2_ButtonStyle::W2_BS_Success).ToSharedRef()
					]
				]
				+ SVerticalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Top)
				.Padding(5.f)
				.AutoHeight()
				[
					SNew(SBox)
					.Visibility_Raw(this, &SSH_FxPointDisplay::GetVis_HasFX)
					[
						USH_WaterfallTool2Statics::CreateButton("Delete FX", FOnClicked::CreateRaw(this, &SSH_FxPointDisplay::ButtonClicked_DeleteFX),ESH_W2_ButtonStyle::W2_BS_Danger).ToSharedRef()
					]
				]
				+ SVerticalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Top)
				.Padding(5.f)
				.AutoHeight()
				[
					SNew(SBox)
					.Visibility_Raw(this, &SSH_FxPointDisplay::GetVis_HasFX)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						[
							USH_WaterfallTool2Statics::CreateButton("Shift FX Up", FOnClicked::CreateRaw(this, &SSH_FxPointDisplay::ButtonClicked_ShiftPointLocation, true),ESH_W2_ButtonStyle::W2_BS_Warning).ToSharedRef()
						]
						+ SHorizontalBox::Slot()
						[
							USH_WaterfallTool2Statics::CreateButton("Shift FX Down", FOnClicked::CreateRaw(this, &SSH_FxPointDisplay::ButtonClicked_ShiftPointLocation, false),ESH_W2_ButtonStyle::W2_BS_Warning).ToSharedRef()
						]
					]
				]
				+ SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Left)
					.VAlign(EVerticalAlignment::VAlign_Center)
					.AutoWidth()
					[
						SNew(SButton)
						.Text(FText::FromString("<"))
						.IsEnabled_Raw(this, &SSH_FxPointDisplay::IsEnabled_FxFocusButton, true)
						.OnClicked_Raw(this, &SSH_FxPointDisplay::ButtonClicked_ChangeFocus, true)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBox)
						.Visibility_Raw(this, &SSH_FxPointDisplay::GetVis_HasFX)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.VAlign(EVerticalAlignment::VAlign_Top)
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text_Raw(this, &SSH_FxPointDisplay::GetText_FocusedComponent)
								.Justification(ETextJustify::Center)
							]
							+ SVerticalBox::Slot()
							[
								DetailsPanel.ToSharedRef()
							]
						]
					]
					+ SHorizontalBox::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Right)
					.VAlign(EVerticalAlignment::VAlign_Center)
					.AutoWidth()
					[
						SNew(SButton)
						.Text(FText::FromString(">"))
						.IsEnabled_Raw(this, &SSH_FxPointDisplay::IsEnabled_FxFocusButton, false)
						.OnClicked_Raw(this, &SSH_FxPointDisplay::ButtonClicked_ChangeFocus, false)
					]
				]
			]
		];
}

ISH_WaterfallFxComponent* SSH_FxPointDisplay::GetTargetFxComp()
{
	return TargetComponents[TargetIndex];
}

ISH_WaterfallFxComponent* SSH_FxPointDisplay::AddComponent_FX(ASH_Waterfall2* InWaterfall, int32 InPathIndex, int32 InPointIndex, ESH_W2_FxType FxType)
{
	USH_WaterfallPathComponent* Path = InWaterfall->GetPathComponent(InPathIndex);
	if (Path)
	{
		//We don't want the whole transform of the spline point because most Fx account for rotation and scale in their system
		//Except yaw because if a particle is spraying forward, its forward should be the forward of the path
		FTransform NewSpawnTransform = FTransform::Identity;
		NewSpawnTransform.SetLocation(Path->GetLocationAtSplinePoint(InPointIndex, ESplineCoordinateSpace::World));
		NewSpawnTransform.SetRotation(FRotator(0.f, Path->GetRotationAtSplinePoint(InPointIndex, ESplineCoordinateSpace::World).Yaw, 0.f).Quaternion());

		switch (FxType)
		{
		case ESH_W2_FxType::VFX: return InWaterfall->AddComponent_VFX(InPathIndex, InPointIndex, NewSpawnTransform);
		case ESH_W2_FxType::SFX: return InWaterfall->AddComponent_SFX(InPathIndex, InPointIndex, NewSpawnTransform);
		}
	}

	return nullptr;
}

FReply SSH_FxPointDisplay::ButtonClicked_AddFX(ESH_W2_FxType FxType)
{
	const FScopedTransaction Transaction(FText::FromString("Add FX"));

	TargetComponents.Add(AddComponent_FX(ParentWaterfall, PathIndex, PointIndex, FxType));

	//Change Target in Details Panel
	TargetIndex = TargetComponents.Num() - 1;
	UpdateFocus();

	return FReply::Handled();
}

FReply SSH_FxPointDisplay::ButtonClicked_DeleteFX()
{
	const FScopedTransaction Transaction(FText::FromString("Delete FX"));

	ParentWaterfall->DeleteComponent_FX(GetTargetFxComp());

	//Change Target in Details Panel
	UpdateTargetComponents();
	UpdateFocus();

	return FReply::Handled();
}

FReply SSH_FxPointDisplay::ButtonClicked_ChangeFocus(bool bLeft)
{
	if (TargetComponents.Num() > 0)
	{
		TargetIndex += bLeft && TargetIndex != INDEX_NONE ? -1 : 1;
	}
	else TargetIndex = INDEX_NONE;

	UpdateFocus();

	return FReply::Handled();
}

FReply SSH_FxPointDisplay::ButtonClicked_ShiftPointLocation(bool bUp)
{
	if (IsEnabled_CanShift(bUp))
	{
		//Change the location and then change the index on the component
		//Get the point location of one point above or below

		USH_WaterfallPathComponent* Path = ParentWaterfall->GetPathComponent(PathIndex);
		if (Path)
		{
			FRotator PrevSplineRotation = Path->GetRotationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);

			Path->SetSimulatedPointHasFX(PointIndex, false);

			PointIndex += (bUp) ? -1 : 1;

			FVector NewLocation = Path->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
			FRotator NewSplineRotation = Path->GetRotationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);

			for (ISH_WaterfallFxComponent* FxComp : TargetComponents)
			{
				if (USceneComponent* ActorComp = FxComp->AsComponent())
				{
					ActorComp->SetWorldLocation(NewLocation);

					FRotator CurrentSplineRotation = ActorComp->GetComponentRotation();
					//The rotation might be altered by the user so the yaw should be rotated to fit with the new direction of the spline point
					//New base yaw + (Old base yaw - user-set yaw)
					FRotator NewRotation = FRotator(CurrentSplineRotation.Pitch, NewSplineRotation.Yaw + (CurrentSplineRotation.Yaw - PrevSplineRotation.Yaw), CurrentSplineRotation.Roll);
					ActorComp->SetWorldRotation(NewRotation);
				}

				FxComp->PointIndex = PointIndex;
			}

			Path->SetSimulatedPointHasFX(PointIndex, true);
		}
	}

	return FReply::Handled();
}

void SSH_FxPointDisplay::UpdateFocus()
{
	DetailsPanel->SetObject(TargetComponents.Num() > 0 && TargetIndex >= 0 ? TargetComponents[FMath::Min(TargetIndex, TargetComponents.Num() - 1)]->AsComponent() : nullptr);

	FocusComponentName = TargetComponents.Num() > 0 && TargetIndex >= 0 ? FText::FromString(TargetComponents[FMath::Min(TargetIndex, TargetComponents.Num() - 1)]->AsComponent()->GetName()) : FText::GetEmpty();
}

EVisibility SSH_FxPointDisplay::GetVis_HasFX() const
{
	USH_WaterfallPathComponent* Path = ParentWaterfall->GetPathComponent(PathIndex);
	bool bHasFX = (Path) ? Path->DoesSimulatedPointHaveFX(PointIndex) : false;
	return bHasFX ? EVisibility::Visible : EVisibility::Hidden;
}

bool SSH_FxPointDisplay::IsEnabled_FxFocusButton(bool bLeft) const
{
	return bLeft ? TargetIndex > 0 : TargetIndex < TargetComponents.Num() - 1;
}

bool SSH_FxPointDisplay::IsEnabled_CanShift(bool bUp)
{
	if (bUp)
	{
		return PointIndex > 0;
	}
	else
	{
		USH_WaterfallPathComponent* Path = ParentWaterfall->GetPathComponent(PathIndex);
		if (Path)
		{
			return PointIndex < Path->GetNumberOfSplinePoints() - 1;
		}
	}

	return false;
}

FText SSH_FxPointDisplay::GetText_FocusedComponent() const
{
	return FocusComponentName;
}

void SSH_FxPointDisplay::UpdateTargetComponents()
{
	if (ParentWaterfall)
	{
		TargetComponents = ParentWaterfall->GetComponents_FX(PathIndex, PointIndex);
		TargetIndex = TargetComponents.Num() > 0 ? 0 : INDEX_NONE;
	}
}