// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "NiagaraSystem.h"
#include "Components/SH_WaterfallVFXComponent.h"

#if WITH_EDITOR
#include "EditorComponents/SH_WaterfallMeshComponent.h"
#include "Actors/SH_Waterfall2.h"
#include "SH_WaterfallTool2Statics.h"
#include "EditorModeManager.h"
#include "Kismet/KismetSystemLibrary.h"
#endif //WITH_EDITOR

USH_WaterfallSettingsComponent::USH_WaterfallSettingsComponent()
{
	//This component doesn't need to tick, it's only for storing variables for the Ed Mode UI
	PrimaryComponentTick.bCanEverTick = false;

#if WITH_EDITOR

	//Find all default Asset Paths
	FSoftObjectPath Path_Curve_BulgeProfile =		FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Curves/Curve_Waterfall_BulgeProfile");
	FSoftObjectPath Path_Curve_Expand =				FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Curves/Curve_Waterfall_Expand");
	FSoftObjectPath Path_Curve_CylinderNormals =	FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Curves/Curve_Waterfall_CylinderNormals");
	FSoftObjectPath Path_Curve_FadeIn_01 =			FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Curves/Curve_Waterfall_FadeIn_01");
	FSoftObjectPath Path_Curve_FadeIn_02 =			FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Curves/Curve_Waterfall_FadeIn_02");
	FSoftObjectPath Path_Curve_FadeIn_03 =			FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Curves/Curve_Waterfall_FadeIn_03");
	FSoftObjectPath Path_Curve_Width_01 =			FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Curves/Curve_Waterfall_Width_01");
	FSoftObjectPath Path_Curve_Width_02 =			FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Curves/Curve_Waterfall_Width_02");
	FSoftObjectPath Path_Curve_Width_03 =			FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Curves/Curve_Waterfall_Width_03");

	FSoftObjectPath Path_MI_SingularMesh =			FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Materials/Instances/Masked/MI_Waterfall_SingularMesh_Masked");
	FSoftObjectPath Path_MI_PerPath =				FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Materials/Instances/Masked/MI_Waterfall_PerPath_Masked");
	FSoftObjectPath Path_MI_Plane =					FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Materials/Instances/Masked/MI_Waterfall_Plane_Masked");
	FSoftObjectPath Path_MI_Splashes =				FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Materials/Instances/Masked/MI_Waterfall_Splashes_Masked");

	FSoftObjectPath Path_TopFx =					FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Particles/Masked/NS_Top_Masked");
	FSoftObjectPath Path_BottomFx =					FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Particles/Masked/NS_Bottom_Masked");
	FSoftObjectPath Path_MiddleFx =					FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Particles/Masked/NS_Middle_Masked");

	//Load default Assets
	UCurveFloat* Curve_BulgeProfile = Cast<UCurveFloat>(Path_Curve_BulgeProfile.TryLoad());
	UCurveFloat* Curve_Expand = Cast<UCurveFloat>(Path_Curve_Expand.TryLoad());
	UCurveFloat* Curve_CylinderNormals = Cast<UCurveFloat>(Path_Curve_CylinderNormals.TryLoad());
	UCurveFloat* Curve_FadeIn_01 = Cast<UCurveFloat>(Path_Curve_FadeIn_01.TryLoad());
	UCurveFloat* Curve_FadeIn_02 = Cast<UCurveFloat>(Path_Curve_FadeIn_02.TryLoad());
	UCurveFloat* Curve_FadeIn_03 = Cast<UCurveFloat>(Path_Curve_FadeIn_03.TryLoad());
	UCurveFloat* Curve_Width_01 = Cast<UCurveFloat>(Path_Curve_Width_01.TryLoad());
	UCurveFloat* Curve_Width_02 = Cast<UCurveFloat>(Path_Curve_Width_02.TryLoad());
	UCurveFloat* Curve_Width_03 = Cast<UCurveFloat>(Path_Curve_Width_03.TryLoad());

	UMaterialInterface* MI_SingularMesh = Cast<UMaterialInterface>(Path_MI_SingularMesh.TryLoad());
	UMaterialInterface* MI_PerPath = Cast<UMaterialInterface>(Path_MI_PerPath.TryLoad());
	UMaterialInterface* MI_Plane = Cast<UMaterialInterface>(Path_MI_Plane.TryLoad());
	UMaterialInterface* MI_Splashes = Cast<UMaterialInterface>(Path_MI_Splashes.TryLoad());

	UNiagaraSystem* PS_Top = Cast<UNiagaraSystem>(Path_TopFx.TryLoad());
	UNiagaraSystem* PS_Bottom = Cast<UNiagaraSystem>(Path_BottomFx.TryLoad());
	UNiagaraSystem* PS_Middle = Cast<UNiagaraSystem>(Path_MiddleFx.TryLoad());

	//Set pointers to default
	CylinderNormalCurve = Curve_CylinderNormals;
	MeshNormalOffsetCurve = Curve_FadeIn_01;
	MeshTraceAlpha = Curve_FadeIn_01;
	PerPathBulgeCurve = Curve_FadeIn_02;
	PerPathBulgeProfileCurve = Curve_BulgeProfile;
	PerPathWidthCurve = Curve_Width_01;
	CrossPlaneWidthCurve = Curve_FadeIn_01;
	//CrossPlaneOffsetCurve = No Default Set
	SingularMeshAdditionalGeoCurve = Curve_FadeIn_01;
	SingularMeshAdditionalGeoSpreadCurve = Curve_Expand;

	SingularMeshMaterial = MI_SingularMesh;
	PerPathMeshMaterial = MI_PerPath;
	CrossMeshMaterial = MI_Plane;
	SplashMeshMaterial = MI_Splashes;

	TopVfxSystem = PS_Top;
	BottomVfxSystem = PS_Bottom;
	MiddleVfxSystem = PS_Middle;

	ButtonClicked_RandomiseSeed();

#endif //WITH_EDITOR
}

#if WITH_EDITOR
ASH_Waterfall2* USH_WaterfallSettingsComponent::GetParentWaterfall()
{
	if (!ParentWaterfall)
	{
		ParentWaterfall = GetOwner<ASH_Waterfall2>();
	}

	return ParentWaterfall;
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_RandomiseSeed()
{
	Seed = FMath::Rand();
	VerifySeedStream();

	return FReply::Handled();
}

TSharedPtr<SWidget> USH_WaterfallSettingsComponent::GetTabActionWidget()
{
	TSharedPtr<SVerticalBox> TabActionWidget = SNew(SVerticalBox);

	switch (CurrentTabState)
	{
	case ESH_W2_TabState::W2_TS_Simple:
	{
		TabActionWidget->AddSlot().Padding(FMargin(1.f))
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().Padding(FMargin(0, 0, .5, 0))
					[
						USH_WaterfallTool2Statics::CreateButton("Generate Paths", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_GeneratePaths), ESH_W2_ButtonStyle::W2_BS_Primary).ToSharedRef()
					]
					+ SHorizontalBox::Slot().Padding(FMargin(.5, 0, 0, 0))
					[
						USH_WaterfallTool2Statics::CreateButton("Clear Paths", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_DeletePaths), ESH_W2_ButtonStyle::W2_BS_Danger).ToSharedRef()
					]
			];

		TabActionWidget->AddSlot().Padding(FMargin(1.f))
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().Padding(FMargin(0, 0, .5, 0))
					[
						USH_WaterfallTool2Statics::CreateButton("Generate Meshes", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_GenerateMeshes), ESH_W2_ButtonStyle::W2_BS_Primary).ToSharedRef()
					]
					+ SHorizontalBox::Slot().Padding(FMargin(.5, 0, 0, 0))
					[
						USH_WaterfallTool2Statics::CreateButton("Clear Meshes", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_ClearDynamicMeshes), ESH_W2_ButtonStyle::W2_BS_Danger).ToSharedRef()
					]
			];

		TabActionWidget->AddSlot().Padding(FMargin(1.f))
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().Padding(FMargin(0, 0, .5, 0))
					[
						USH_WaterfallTool2Statics::CreateButton("Generate VFX", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_GenerateFx), ESH_W2_ButtonStyle::W2_BS_Primary).ToSharedRef()
					]
					+ SHorizontalBox::Slot().Padding(FMargin(0, 0, .5, 0))
					[
						USH_WaterfallTool2Statics::CreateButton("Bake to Static Mesh", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_BakeToStaticMesh), ESH_W2_ButtonStyle::W2_BS_Success).ToSharedRef()
					]
			];

		break;
	}
	case ESH_W2_TabState::W2_TS_Paths:
	{
		TabActionWidget->AddSlot().Padding(FMargin(1.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(FMargin(0, 0, .5, 0))
				[
					USH_WaterfallTool2Statics::CreateButton("Generate Paths", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_GeneratePaths), ESH_W2_ButtonStyle::W2_BS_Primary).ToSharedRef()
				]
				+ SHorizontalBox::Slot().Padding(FMargin(.5, 0, 0, 0))
				[
					USH_WaterfallTool2Statics::CreateButton("Clear Paths", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_DeletePaths), ESH_W2_ButtonStyle::W2_BS_Danger).ToSharedRef()
				]
			];

		break;
	}
	case ESH_W2_TabState::W2_TS_Meshes:
	{
		TabActionWidget->AddSlot().Padding(FMargin(1.f))
			[
				USH_WaterfallTool2Statics::CreateButton("Generate Meshes", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_GenerateMeshes), ESH_W2_ButtonStyle::W2_BS_Primary).ToSharedRef()
			];

		TabActionWidget->AddSlot().Padding(FMargin(1.f))
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().Padding(FMargin(0, 0, .5, 0))
					[
						USH_WaterfallTool2Statics::CreateButton("Clear Selected Mesh", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_ClearSpecificMesh), ESH_W2_ButtonStyle::W2_BS_Danger).ToSharedRef()
					]
					+ SHorizontalBox::Slot().Padding(FMargin(.5, 0, 0, 0))
					[
						USH_WaterfallTool2Statics::CreateButton("Clear Meshes", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_ClearDynamicMeshes), ESH_W2_ButtonStyle::W2_BS_Danger).ToSharedRef()
					]
			];

		break;
	}
	case ESH_W2_TabState::W2_TS_Bake:
	{
		TabActionWidget->AddSlot().Padding(FMargin(1.f))
			[
				USH_WaterfallTool2Statics::CreateButton("Bake to Static Mesh", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_BakeToStaticMesh), ESH_W2_ButtonStyle::W2_BS_Primary).ToSharedRef()
			];

		break;
	}
	case ESH_W2_TabState::W2_TS_FX:
	{
		TabActionWidget->AddSlot().Padding(FMargin(1.f))
			[
				USH_WaterfallTool2Statics::CreateButton("Generate VFX", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_GenerateFx), ESH_W2_ButtonStyle::W2_BS_Primary).ToSharedRef()
			];

		break;
	};
	case ESH_W2_TabState::W2_TS_Debug:
	{
		TabActionWidget->AddSlot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().Padding(FMargin(1.f))
				[
					USH_WaterfallTool2Statics::CreateButton("Debug: Show Vertices", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_Debug_ShowVertices), ESH_W2_ButtonStyle::W2_BS_Info).ToSharedRef()
				]
				+ SVerticalBox::Slot().Padding(FMargin(1.f))
				[
					USH_WaterfallTool2Statics::CreateButton("Debug: Show Triangles", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_Debug_ShowTriangles), ESH_W2_ButtonStyle::W2_BS_Info).ToSharedRef()
				]
				+ SVerticalBox::Slot().Padding(FMargin(1.f))
				[
					USH_WaterfallTool2Statics::CreateButton("Debug: Show Directions", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_Debug_ShowDirections), ESH_W2_ButtonStyle::W2_BS_Info).ToSharedRef()
				]			
				+ SVerticalBox::Slot().Padding(FMargin(1.f))
				[
					USH_WaterfallTool2Statics::CreateButton("Debug: Print Remapped Points", FOnClicked::CreateUObject(this, &USH_WaterfallSettingsComponent::ButtonClicked_Debug_PrintRemappedPoints), ESH_W2_ButtonStyle::W2_BS_Info).ToSharedRef()
				]
			];

		break;
	}
	}

	return TabActionWidget;
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_GeneratePaths()
{
	//Set the whole widget disabled while we're processing so that the user can't modify things which could break processing
	SetToolkitWidgetDisabled(FSimpleDelegate::CreateUObject(this, &USH_WaterfallSettingsComponent::CancelGeneration_Paths));
	StartSimulate(FSimpleDelegate::CreateUObject(this, &USH_WaterfallSettingsComponent::SetToolkitWidgetEnabled));

	return FReply::Handled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_DeletePaths()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString("Are you sure you want to delete all generated paths?\n(This will also clear all dynamic mesh data.)"), FText::FromString("Delete All Paths"))
		== EAppReturnType::Yes)
	{
		//Remove any existing paths
		if (GetParentWaterfall())
		{
			const FScopedTransaction Transaction(FText::FromString("Delete Paths"));

			GetParentWaterfall()->GetPathBuilder().DeleteAllPaths();
		}
	}

	return FReply::Handled();
}

void USH_WaterfallSettingsComponent::CancelGeneration_Paths()
{
	CancelSimulate();
	SetToolkitWidgetEnabled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_GenerateMeshes()
{
	//Set the whole widget disabled while we're processing so that the user can't modify things which could break processing
	SetToolkitWidgetDisabled(FSimpleDelegate::CreateUObject(this, &USH_WaterfallSettingsComponent::CancelGeneration_Meshes));
	StartMeshGeneration(FSimpleDelegate::CreateUObject(this, &USH_WaterfallSettingsComponent::SetToolkitWidgetEnabled));

	return FReply::Handled();
}

void USH_WaterfallSettingsComponent::CancelGeneration_Meshes()
{
	CancelMeshGeneration();
	SetToolkitWidgetEnabled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_ClearDynamicMeshes()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString("Are you sure you want to clear all dynamic mesh data?"), FText::FromString("Clear All Dynamic Meshes"))
		== EAppReturnType::Yes)
	{
		SetToolkitWidgetDisabled();
		ClearDynamicMeshes(ESH_MeshGenerationType::W2_MG_All, FSimpleDelegate::CreateUObject(this, &USH_WaterfallSettingsComponent::SetToolkitWidgetEnabled));
	}

	return FReply::Handled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_ClearSpecificMesh()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString("Are you sure you want to clear the dynamic mesh data for the " + StringHelpers::ToString(SelectedMesh) + " mesh?"), FText::FromString("Clear Dynamic Mesh"))
		== EAppReturnType::Yes)
	{
		SetToolkitWidgetDisabled();
		ClearDynamicMeshes(SelectedMesh, FSimpleDelegate::CreateUObject(this, &USH_WaterfallSettingsComponent::SetToolkitWidgetEnabled));
	}

	return FReply::Handled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_BakeToStaticMesh()
{
	SetToolkitWidgetDisabled();
	BakeToStaticMesh(FSimpleDelegate::CreateUObject(this, &USH_WaterfallSettingsComponent::SetToolkitWidgetEnabled));

	return FReply::Handled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_GenerateFx()
{
	FTransform FxAttachPointTransform = ParentWaterfall->GetFxAttachPoint()->GetComponentTransform();
	USH_WaterfallVFXComponent* WorkingFxComp = GetParentWaterfall()->GetVFX_Top();
	if (WorkingFxComp)
	{
		if (bTopVFX)
		{
			WorkingFxComp->SetAsset(TopVfxSystem);
			WorkingFxComp->PointData = GetParentWaterfall()->GetAllStartVfxData();
			WorkingFxComp->UpdateComponentParams();
		}
		else
		{
			WorkingFxComp->SetAsset(nullptr);
		}
	}

	WorkingFxComp = GetParentWaterfall()->GetVFX_Bottom();
	if (WorkingFxComp)
	{
		if (bBottomVFX)
		{
			WorkingFxComp->SetAsset(BottomVfxSystem);
			WorkingFxComp->PointData = GetParentWaterfall()->GetAllEndVfxData();
			WorkingFxComp->UpdateComponentParams();
		}
		else
		{
			WorkingFxComp->SetAsset(nullptr);
		}
	}

	WorkingFxComp = GetParentWaterfall()->GetVFX_Middle();
	if (WorkingFxComp)
	{
		if (bMiddleVFX)
		{
			WorkingFxComp->SetAsset(MiddleVfxSystem);

			if (bMiddleUsesDistance) WorkingFxComp->PointData = GetParentWaterfall()->GetAllVfxDataInRange(DistanceRange, RemapPoints);
			else WorkingFxComp->PointData = GetParentWaterfall()->GetAllVfxDataInRange(PointRange);

			WorkingFxComp->UpdateComponentParams();
		}
		else
		{
			WorkingFxComp->SetAsset(nullptr);
		}
	}

	return FReply::Handled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_Debug_ShowVertices()
{
	if (GetParentWaterfall())
	{
		GetParentWaterfall()->GetMeshBuilder().Debug_ShowVertices(DebugPointsDuration, DebugPointsShowOnlyThesePoints);
	}

	return FReply::Handled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_Debug_ShowTriangles()
{
	if (GetParentWaterfall())
	{
		GetParentWaterfall()->GetMeshBuilder().Debug_ShowTriangles(DebugPointsDuration);
	}

	return FReply::Handled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_Debug_ShowDirections()
{
	if (GetParentWaterfall())
	{
		GetParentWaterfall()->GetMeshBuilder().Debug_ShowDirections(DebugPointsDuration);
	}

	return FReply::Handled();
}

FReply USH_WaterfallSettingsComponent::ButtonClicked_Debug_PrintRemappedPoints()
{
	if (GetParentWaterfall())
	{
		GetParentWaterfall()->GetMeshBuilder().Debug_PrintRemappedPaths();
	}

	return FReply::Handled();
}

void USH_WaterfallSettingsComponent::SetWidgetEnabled(bool bEnabled, FSimpleDelegate OptionalCancelDelegate)
{
	GetParentWaterfall()->OnEnableUI.ExecuteIfBound(bEnabled, OptionalCancelDelegate);
}

void USH_WaterfallSettingsComponent::VerifySeedStream()
{
	//Make sure the stream is correctly initialised from the current seed
	if (SeedStream.GetInitialSeed() != Seed)
	{
		SeedStream = FRandomStream(Seed);
	}
}

int32 USH_WaterfallSettingsComponent::GetNumPaths()
{
	//if (IsSimple()) return 3;
	return NumPaths;
}

int32 USH_WaterfallSettingsComponent::GetSubsteps()
{
	if (IsSimple()) return 50;
	return Substeps;
}

float USH_WaterfallSettingsComponent::GetSimulationDeltaTime()
{
	//if (IsSimple()) return 0.1f;
	return SimulationDeltaTime;
}

bool USH_WaterfallSettingsComponent::GetAddSpawnJitter()
{
	return bAddSpawnJitter;
}

float USH_WaterfallSettingsComponent::GetSpawnTraceDistance()
{
	if (IsSimple()) return 100.f;
	return SpawnTraceDistance;
}

float USH_WaterfallSettingsComponent::GetSpawnTraceOffset()
{
	if (IsSimple()) return 1.f;
	return SpawnTraceOffset;
}

float USH_WaterfallSettingsComponent::GetSpawnTraceAlpha()
{
	if (IsSimple()) return 1.f;
	return SpawnTraceAlpha;
}

TEnumAsByte<ETraceTypeQuery> USH_WaterfallSettingsComponent::GetSimulationTraceChannel()
{
	if (IsSimple()) return ETraceTypeQuery::TraceTypeQuery1; //1 is visibility
	return SimulationTraceChannel;
}

int32 USH_WaterfallSettingsComponent::GetPathRangeMinChecked()
{
	return FMath::Max(FMath::Min(PathRange.X, PathRange.Y), 0);
}

int32 USH_WaterfallSettingsComponent::GetPathRangeMaxChecked(int32 LastIndex)
{
	return FMath::Min(FMath::Max(PathRange.X, PathRange.Y), LastIndex);
}

float USH_WaterfallSettingsComponent::GetSamplePathMinChecked()
{
	return FMath::Min(SamplePath.X, SamplePath.Y);
}

float USH_WaterfallSettingsComponent::GetSamplePathMaxChecked()
{
	return FMath::Max(SamplePath.X, SamplePath.Y);
}

float USH_WaterfallSettingsComponent::GetSingularMeshAdditionalGeo()
{
	if (IsSimple()) return 0.f;

	return SingularMeshAdditionalGeo;
}

UMaterialInterface* USH_WaterfallSettingsComponent::GetMaterialForMesh()
{
	switch (SelectedMesh)
	{
	case ESH_MeshGenerationType::W2_MG_Singular:	return SingularMeshMaterial;
	case ESH_MeshGenerationType::W2_MG_PerPath:		return PerPathMeshMaterial;
	case ESH_MeshGenerationType::W2_MG_Cross:		return CrossMeshMaterial;
	case ESH_MeshGenerationType::W2_MG_Splash:		return SplashMeshMaterial;
	}

	return nullptr;
}

void USH_WaterfallSettingsComponent::UpdateCurrentTabState(ESH_W2_TabState NewTabState)
{
	if (CurrentTabState != NewTabState)
	{
		if (GetParentWaterfall())
		{
			GetParentWaterfall()->SetPathsVisibility(
				!GetParentWaterfall()->HasGeneratedMeshes()
				|| NewTabState == ESH_W2_TabState::W2_TS_Paths
				|| NewTabState == ESH_W2_TabState::W2_TS_Simple
				|| NewTabState == ESH_W2_TabState::W2_TS_Debug
				|| NewTabState == ESH_W2_TabState::W2_TS_FX
			);

			GetParentWaterfall()->SetMeshesVisibility(
				NewTabState == ESH_W2_TabState::W2_TS_Meshes
				|| NewTabState == ESH_W2_TabState::W2_TS_Simple
				|| NewTabState == ESH_W2_TabState::W2_TS_Debug
				|| NewTabState == ESH_W2_TabState::W2_TS_FX
				|| NewTabState == ESH_W2_TabState::W2_TS_Materials
				|| NewTabState == ESH_W2_TabState::W2_TS_Bake
			);

			GetParentWaterfall()->SetFxVisibility(NewTabState != ESH_W2_TabState::W2_TS_Paths);
			GetParentWaterfall()->GetBakedMeshComp()->SetVisibility(false);
		}

		CurrentTabState = NewTabState;
	}
}

void USH_WaterfallSettingsComponent::UpdateMaterialForComponent(ESH_MeshGenerationType MeshType, UMaterialInterface* NewMaterial)
{
	if (GetParentWaterfall())
	{
		USH_WaterfallMeshComponent* WaterfallMeshComp = GetParentWaterfall()->GetMeshForType(MeshType);
		if (WaterfallMeshComp)
		{
			WaterfallMeshComp->SetMaterial(0, NewMaterial);
			WaterfallMeshComp->MarkRenderStateDirty(); //Need to make sure the Viewport updates the material in the next rendered frame
		}

		//Also update the material in the Static Mesh if desired
		if (bUpdateMaterialsInBakedMeshToo)
		{
			UStaticMeshComponent* BakedMeshComp = GetParentWaterfall()->GetBakedMeshComp();
			TArray<FName> MaterialSlotNames = BakedMeshComp->GetMaterialSlotNames();
			for (int i = 0; i < MaterialSlotNames.Num(); i++)
			{
				if (MaterialSlotNames[i].ToString() == "_" + StringHelpers::ToString(MeshType))
				{
					BakedMeshComp->SetMaterialByName(MaterialSlotNames[i], NewMaterial);
				}
			}
		}
	}
}

void USH_WaterfallSettingsComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SelectedMesh)
		|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SimpleGenerateSingleMesh)
		|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SimpleGeneratePerPathMesh)
		|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SimpleGenerateSplashMesh)
		|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SimpleGeneratePlaneMesh)
		|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SimpleGeneratePlaneMesh)
		|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bTopVFX)
		|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bMiddleVFX)
		|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bBottomVFX)
		|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bMiddleUsesDistance)
		)
	{
		RefreshDetailsPanel.ExecuteIfBound();
	}
	else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, NumPaths))
	{
		if (NumPaths < 1) NumPaths = 1;
		RefreshDetailsPanel.ExecuteIfBound();
	}
	//If we update materials, we also need to update it on any dynamic mesh components associated with it
	else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SingularMeshMaterial))
	{
		UpdateMaterialForComponent(ESH_MeshGenerationType::W2_MG_Singular, SingularMeshMaterial);
	}
	//If we update materials, we also need to update it on any dynamic mesh components associated with it
	else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, PerPathMeshMaterial))
	{
		UpdateMaterialForComponent(ESH_MeshGenerationType::W2_MG_PerPath, PerPathMeshMaterial);
	}
	//If we update materials, we also need to update it on any dynamic mesh components associated with it
	else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, CrossMeshMaterial))
	{
		UpdateMaterialForComponent(ESH_MeshGenerationType::W2_MG_Cross, CrossMeshMaterial);
	}
	//If we update materials, we also need to update it on any dynamic mesh components associated with it
	else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SplashMeshMaterial))
	{
		UpdateMaterialForComponent(ESH_MeshGenerationType::W2_MG_Splash, SplashMeshMaterial);
	}
	else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, FxPathSelected))
	{
		//Clamp it between 0 and NumPaths - 1
		if (GetParentWaterfall())
		{
			if (GetParentWaterfall()->HasGeneratedPaths())
			{
				FxPathSelected = FMath::Clamp<int32>(FxPathSelected, 0, GetParentWaterfall()->NumPaths() - 1);
				RefreshDetailsPanel.ExecuteIfBound();
			}
			else
			{
				FxPathSelected = 0;
			}
		}
		else
		{
			FxPathSelected = 0;
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void USH_WaterfallSettingsComponent::StartSimulate(FSimpleDelegate OnProcessingFinished)
{
	VerifySeedStream();

	//OnProcessingFinished will reactivate the buttons
	if (GetParentWaterfall())
	{
		if (!GetParentWaterfall()->IsProcessing())
		{
			GetParentWaterfall()->GetPathBuilder().StartSimulate(OnProcessingFinished);
		}
	}
	else OnProcessingFinished.ExecuteIfBound();
}

void USH_WaterfallSettingsComponent::CancelSimulate()
{
	if (GetParentWaterfall())
	{
		GetParentWaterfall()->GetPathBuilder().CancelSimulate();
	}
}

void USH_WaterfallSettingsComponent::StartMeshGeneration(FSimpleDelegate OnProcessingFinished)
{
	VerifySeedStream();

	//OnProcessingFinished will reactivate the buttons
	if (GetParentWaterfall())
	{
		if (!GetParentWaterfall()->IsProcessing())
		{
			//Set up the mesh generation queue
			if (!IsSimple())
			{
				GetParentWaterfall()->GetMeshBuilder().QueuedMeshesToBeGenerated.Add(SelectedMesh);
			}
			else
			{
				if (SimpleGenerateSingleMesh) GetParentWaterfall()->GetMeshBuilder().QueuedMeshesToBeGenerated.Add(ESH_MeshGenerationType::W2_MG_Singular);
				if (SimpleGeneratePerPathMesh) GetParentWaterfall()->GetMeshBuilder().QueuedMeshesToBeGenerated.Add(ESH_MeshGenerationType::W2_MG_PerPath);
				if (SimpleGenerateSplashMesh) GetParentWaterfall()->GetMeshBuilder().QueuedMeshesToBeGenerated.Add(ESH_MeshGenerationType::W2_MG_Splash);
				if (SimpleGeneratePlaneMesh) GetParentWaterfall()->GetMeshBuilder().QueuedMeshesToBeGenerated.Add(ESH_MeshGenerationType::W2_MG_Cross);
			}

			GetParentWaterfall()->GetMeshBuilder().StartMeshGeneration(OnProcessingFinished);
		}
	}
	else OnProcessingFinished.ExecuteIfBound();
}

void USH_WaterfallSettingsComponent::CancelMeshGeneration()
{
	if (GetParentWaterfall())
	{
		GetParentWaterfall()->GetMeshBuilder().CancelMeshGeneration();
	}
}

void USH_WaterfallSettingsComponent::ClearDynamicMeshes(ESH_MeshGenerationType MeshType, FSimpleDelegate OnProcessingFinished)
{
	if (GetParentWaterfall())
	{
		if (!GetParentWaterfall()->IsProcessing())
		{
			const FScopedTransaction Transaction(FText::FromString("Clear Dynamic Mesh: " + StringHelpers::ToString(MeshType)));

			GetParentWaterfall()->ClearDynamicMeshes(MeshType);
			//If there is no mesh data, then the paths should be visible
			GetParentWaterfall()->SetPathsVisibility(true);
		}
	}

	OnProcessingFinished.ExecuteIfBound();
}

void USH_WaterfallSettingsComponent::BakeToStaticMesh(FSimpleDelegate OnProcessingFinished)
{
	if (GetParentWaterfall())
	{
		if (!GetParentWaterfall()->IsProcessing())
		{
			GetParentWaterfall()->GetStaticBuilder().BakeToStaticMesh(OnProcessingFinished);
		}
	}
	else OnProcessingFinished.ExecuteIfBound();
}

bool USH_WaterfallSettingsComponent::IsSimple()
{
	return CurrentTabState == ESH_W2_TabState::W2_TS_Simple;
}

bool USH_WaterfallSettingsComponent::HasGeneratedMeshes()
{
	if (GetParentWaterfall()) return GetParentWaterfall()->HasGeneratedMeshes();

	return false;
}

#endif //WITH_EDITOR