// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Actors/SH_Waterfall2.h"
#include "Components/SH_WaterfallVFXComponent.h"
#include "Components/SH_WaterfallSFXComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EditorComponents/SH_WaterfallPathComponent.h"
#include "EditorComponents/SH_WaterfallMeshComponent.h"
#include "SH_WaterfallTool2Statics.h"
#include "TimerManager.h"

#if WITH_EDITOR
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Kismet/KismetMathLibrary.h"
#endif

ASH_Waterfall2::ASH_Waterfall2()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root Comp"));
	SetRootComponent(RootComp);

	FxAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FxAttachPoint"));
	if (FxAttachPoint) FxAttachPoint->SetupAttachment(GetRootComponent());

	VFX_Top = CreateDefaultSubobject<USH_WaterfallVFXComponent>(TEXT("Top"));
	if (VFX_Top) VFX_Top->SetupAttachment(FxAttachPoint);
	VFX_Bottom = CreateDefaultSubobject<USH_WaterfallVFXComponent>(TEXT("Bottom"));
	if (VFX_Bottom) VFX_Bottom->SetupAttachment(FxAttachPoint);
	VFX_Middle = CreateDefaultSubobject<USH_WaterfallVFXComponent>(TEXT("Middle"));
	if (VFX_Middle) VFX_Middle->SetupAttachment(FxAttachPoint);

	SFX_Top = CreateDefaultSubobject<USH_WaterfallSFXComponent>(TEXT("SFX Top"));
	if (SFX_Top) SFX_Top->SetupAttachment(FxAttachPoint);
	SFX_Middle = CreateDefaultSubobject<USH_WaterfallSFXComponent>(TEXT("SFX Mid"));
	if (SFX_Middle) SFX_Middle->SetupAttachment(FxAttachPoint);
	SFX_Bottom = CreateDefaultSubobject<USH_WaterfallSFXComponent>(TEXT("SFX Bottom"));
	if (SFX_Bottom) SFX_Bottom->SetupAttachment(FxAttachPoint);

	BakedMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(FName("Mesh Comp"));
	if (BakedMeshComp)
	{
		BakedMeshComp->SetupAttachment(GetRootComponent());
		BakedMeshComp->SetVisibility(false);
		BakedMeshComp->SetHiddenInGame(false);
	}

	MeshComp_Singular = CreateDefaultSubobject<USH_WaterfallMeshComponent>(FName("Dynamic Mesh: Singular"));
	if (MeshComp_Singular)
	{
		MeshComp_Singular->SetupAttachment(GetRootComponent());
		MeshComp_Singular->MeshType = ESH_MeshGenerationType::W2_MG_Singular;
		MeshComp_Singular->SetVisibility(true);
		MeshComp_Singular->SetHiddenInGame(false);
		MeshComp_Singular->SetCastShadow(true);
		MeshComp_Singular->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
	}

	MeshComp_PerPath = CreateDefaultSubobject<USH_WaterfallMeshComponent>(FName("Dynamic Mesh: Per Path"));
	if (MeshComp_PerPath)
	{
		MeshComp_PerPath->SetupAttachment(GetRootComponent());
		MeshComp_PerPath->MeshType = ESH_MeshGenerationType::W2_MG_PerPath;
		MeshComp_PerPath->SetVisibility(true);
		MeshComp_PerPath->SetHiddenInGame(false);
		MeshComp_PerPath->SetCastShadow(true);
		MeshComp_PerPath->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
	}

	MeshComp_Cross = CreateDefaultSubobject<USH_WaterfallMeshComponent>(FName("Dynamic Mesh: Cross"));
	if (MeshComp_Cross)
	{
		MeshComp_Cross->SetupAttachment(GetRootComponent());
		MeshComp_Cross->MeshType = ESH_MeshGenerationType::W2_MG_Cross;
		MeshComp_Cross->SetVisibility(true);
		MeshComp_Cross->SetHiddenInGame(false);
		MeshComp_Cross->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
	}

	MeshComp_Splash = CreateDefaultSubobject<USH_WaterfallMeshComponent>(FName("Dynamic Mesh: Splash"));
	if (MeshComp_Splash)
	{
		MeshComp_Splash->SetupAttachment(GetRootComponent());
		MeshComp_Splash->MeshType = ESH_MeshGenerationType::W2_MG_Splash;
		MeshComp_Splash->SetVisibility(true);
		MeshComp_Splash->SetHiddenInGame(false);
		//MeshComp_Splash->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
	}

#if WITH_EDITOR
	//All of these components are Editor-Only so we need to use CreateEditorOnlyDefaultSubobject otherwise if WITH_EDITORONLY_DATA will cause an error when building
	//CreateEditorOnlyDefaultSubobject makes it so that it's not visible in the Details Panel

	TopSpline = CreateDefaultSubobject<USplineComponent>(FName("Top Spline"));
	if (TopSpline)
	{
		TopSpline->SetupAttachment(GetRootComponent());
		TopSpline->SetRelativeLocation(FVector(0.f, -200.f, 500.f));
		TopSpline->SetLocationAtSplinePoint(0, FVector(-300.f, 0, 0), ESplineCoordinateSpace::Local);
		TopSpline->SetLocationAtSplinePoint(1, FVector(300.f, 0, 0), ESplineCoordinateSpace::Local);
		TopSpline->SetUnselectedSplineSegmentColor(FLinearColor::Blue);
	}

	KillPlaneVisualiser = CreateDefaultSubobject<UStaticMeshComponent>(FName("Kill Plane"));
	if (KillPlaneVisualiser)
	{
		FSoftObjectPath Path_PlaneMesh = FSoftObjectPath("/Engine/BasicShapes/Plane");
		KillPlaneVisualiser->SetStaticMesh(Cast<UStaticMesh>(Path_PlaneMesh.TryLoad()));

		KillPlaneVisualiser->SetupAttachment(GetRootComponent());
		KillPlaneVisualiser->SetRelativeScale3D(FVector(20.f)); //A scale of 20 units will put it at 1000x1000 units because by default the mesh is 50x50 units
		KillPlaneVisualiser->SetHiddenInGame(true);
		KillPlaneVisualiser->SetVisibility(false);

		//Set the kill plane material
		FSoftObjectPath Path_MI_KillPlane = FSoftObjectPath("/Shadersource_WaterfallTool2/Base/Materials/Data/MI_KillArea");
		UMaterialInterface* MI_KillPlane = Cast<UMaterialInterface>(Path_MI_KillPlane.TryLoad());
		if (MI_KillPlane)
			KillPlaneVisualiser->SetMaterial(0, MI_KillPlane);
	}

	WaterfallSettingsComp = CreateEditorOnlyDefaultSubobject<USH_WaterfallSettingsComponent>(FName("Waterfall Settings"));

	PathBuilder = FSH_WaterfallBuilder_Path(this);
	MeshBuilder = FSH_WaterfallBuilder_Mesh(this);
	StaticBuilder = FSH_WaterfallBuilder_Static(this);
#endif
}

void ASH_Waterfall2::BeginPlay()
{
	Super::BeginPlay();

	//Make sure the baked mesh is visible, in case it's been saved as hidden in the Editor Mode by mistake
	SetMeshesVisibility(bShowDynamicMeshesInGame);

	// If we're not using the dynamic meshes, we should clear them so that extra memory isn't needed for all of their data
	// (this only affects the runtime copy of the actor, not the editor actor)
	if (!bShowDynamicMeshesInGame)
	{
		ClearDynamicMeshes(ESH_MeshGenerationType::W2_MG_All);
	}

	//Make sure the niagara systems have the correct variables in case they've changed and/or haven't been set correctly in editor
	//And make sure they're visible
	TArray<ISH_WaterfallFxComponent*> FxComps = GetAllComponents_FX(true);
	for (ISH_WaterfallFxComponent* FxComp : FxComps)
	{
		FxComp->AsComponent()->SetVisibility(true);
		FxComp->UpdateComponentParams();
	}
}

void ASH_Waterfall2::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	//Set the kill plane scale and location XY to the top spline bounds scale and origin
	FBoxSphereBounds CalculatedSplineBounds = GetTopSpline()->CalcBounds(GetActorTransform());
	FVector BoundsToScale = (CalculatedSplineBounds.BoxExtent / 120) * 2.5;
	if (GetKillPlane() && GetWaterfallSettings())
	{
		GetKillPlane()->SetWorldScale3D(FVector(20) + BoundsToScale + GetWaterfallSettings()->KillPlaneScaleOffset);
		GetKillPlane()->SetWorldLocation(FVector(CalculatedSplineBounds.Origin.X, CalculatedSplineBounds.Origin.Y, GetKillPlane()->GetComponentLocation().Z));
	}

	//Update the directional arrows
	UpdateArrowComponents();
#endif //WITH_EDITOR
}

#if WITH_EDITOR
void ASH_Waterfall2::UpdateArrowComponents()
{
	//Clean up any previous UArrowComponents
	{
		TArray<UArrowComponent*> ArrowComps = {};
		GetComponents<UArrowComponent>(ArrowComps);

		GEngine->ForceGarbageCollection(true); //So that the names can be released before any other paths are created, in case GC is slow

		for (int i = ArrowComps.Num() - 1; i >= 0; i--)
		{
			if (ArrowComps[i]->GetName().Contains("GeneratedArrow_"))
			{
				FString NewName = "DestroyedArrow_";
				FName UniqueName = MakeUniqueObjectName(ArrowComps[i]->GetOuter(), USH_WaterfallPathComponent::StaticClass(), FName(NewName));
				ArrowComps[i]->Rename(*UniqueName.ToString(), ArrowComps[i]->GetOuter());

				ArrowComps[i]->DestroyComponent();
			}
		}
	}

	if (GetWaterfallSettings())
	{
		if (GetWaterfallSettings()->bShowArrowComponents)
		{
			for (int i = 0; i < GetWaterfallSettings()->GetNumPaths(); i++)
			{
				float CalculatedNormalisedSplineDistance = (GetWaterfallSettings()->GetNumPaths() > 1)
					? FMath::GetMappedRangeValueClamped(TRange<float>(0.f, 1.f), TRange<float>(GetWaterfallSettings()->GetPathSpawnRange().X, GetWaterfallSettings()->GetPathSpawnRange().Y), (float)i / FMath::Max<float>(GetWaterfallSettings()->GetNumPaths() - 1, 1))
					: GetWaterfallSettings()->GetSinglePathPosition();

				UArrowComponent* NewArrowComponent = NewObject<UArrowComponent>(this, FName("GeneratedArrow_" + FString::SanitizeFloat(i, 0)));
				if (NewArrowComponent)
				{
					NewArrowComponent->SetupAttachment(GetTopSpline());
					NewArrowComponent->MarkAsEditorOnlySubobject(); //This component is Editor-Only and should not be included in packaged builds - this makes it so that it doesn't show up in the Details Panel
					AddInstanceComponent(NewArrowComponent);
					AddOwnedComponent(NewArrowComponent);
					NewArrowComponent->RegisterComponent();

					//The new component is attached to the spline, so we need to work in local space
					FRotator NewRotation = UKismetMathLibrary::MakeRotationFromAxes(
						GetTopSpline()->GetRightVectorAtTime(CalculatedNormalisedSplineDistance, ESplineCoordinateSpace::Local, true),
						GetTopSpline()->GetDirectionAtTime(CalculatedNormalisedSplineDistance, ESplineCoordinateSpace::Local, true),
						GetTopSpline()->GetUpVectorAtTime(CalculatedNormalisedSplineDistance, ESplineCoordinateSpace::Local, true)
					);

					//If the spline direction is flipped, the Yaw needs to rotate 180 degrees
					if (GetWaterfallSettings()->bFlipSplineDirection)
					{
						NewRotation += FRotator(0.f, 180.f, 0.f);
					}

					FTransform LocalComponentTransform = FTransform(
						NewRotation,
						GetTopSpline()->GetLocationAtTime(CalculatedNormalisedSplineDistance, ESplineCoordinateSpace::Local, true),
						FVector::OneVector
					);

					NewArrowComponent->SetRelativeTransform(LocalComponentTransform);

					NewArrowComponent->SetArrowLength(GetWaterfallSettings()->ArrowComponentLength);
				}
			}
		}
	}
}
#endif //WITH_EDITOR

void ASH_Waterfall2::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if WITH_EDITOR
	if (PathBuilder.IsProcessing()) PathBuilder.Tick(DeltaTime);
	else if (MeshBuilder.IsProcessing()) MeshBuilder.Tick(DeltaTime);
	else if (StaticBuilder.IsProcessing()) StaticBuilder.Tick(DeltaTime);
#endif //WITH_EDITOR
}

void ASH_Waterfall2::SetStaticMesh(UStaticMesh* NewMesh)
{
	if (BakedMeshComp)
	{
		BakedMeshComp->SetStaticMesh(NewMesh);
		OnStaticMeshPropChange();
	}
}

#if WITH_EDITOR
TArray<USH_WaterfallPathComponent*> ASH_Waterfall2::GetAllPathComponents()
{
	TArray<USH_WaterfallPathComponent*> WaterfallPaths = {};
	GetComponents<USH_WaterfallPathComponent>(WaterfallPaths);
	WaterfallPaths.Sort();
	return WaterfallPaths;
}

USH_WaterfallPathComponent* ASH_Waterfall2::GetPathComponent(int32 PathIndex)
{
	for (USH_WaterfallPathComponent* PathComp : GetAllPathComponents())
	{
		if (PathComp->PathIndex == PathIndex)
		{
			return PathComp;
		}
	}

	return nullptr;
}

int32 ASH_Waterfall2::NumPaths()
{
	return GetAllPathComponents().Num();
}

bool ASH_Waterfall2::HasGeneratedPaths()
{
	return NumPaths() > 0;
}

void ASH_Waterfall2::SetPathsVisibility(bool bVisible)
{
	for (USH_WaterfallPathComponent* PathComp : GetAllPathComponents())
	{
		PathComp->SetVisibility(bVisible);
		PathComp->SetDrawDebug(bVisible);
	}
}

bool ASH_Waterfall2::HasGeneratedMeshes()
{
	for (USH_WaterfallMeshComponent* MeshComp : GetAllMeshComponents())
	{
		if (MeshComp->GetDynamicMesh())
		{
			if (MeshComp->GetDynamicMesh()->GetTriangleCount() > 0) return true;
		}
	}

	return false;
}

#endif
TArray<USH_WaterfallMeshComponent*> ASH_Waterfall2::GetAllMeshComponents()
{
	TArray<USH_WaterfallMeshComponent*> MeshComponents = {};
	GetComponents<USH_WaterfallMeshComponent>(MeshComponents);
	return MeshComponents;
}

void ASH_Waterfall2::SetMeshesVisibility(bool bShowDynamicMeshes)
{
	if (BakedMeshComp) BakedMeshComp->SetVisibility(!bShowDynamicMeshes);

	for (USH_WaterfallMeshComponent* MeshComp : GetAllMeshComponents())
	{
		if (MeshComp) MeshComp->SetVisibility(bShowDynamicMeshes);
	}
}

void ASH_Waterfall2::ClearDynamicMeshes(ESH_MeshGenerationType MeshType)
{
	TArray<USH_WaterfallMeshComponent*> MeshComponents = GetAllMeshComponents();

	for (int i = MeshComponents.Num() - 1; i >= 0; i--)
	{
		if (MeshType == ESH_MeshGenerationType::W2_MG_All || MeshComponents[i]->MeshType == MeshType)
		{
			MeshComponents[i]->GetDynamicMesh()->Reset();
		}
	}
}
#if WITH_EDITOR

USH_WaterfallMeshComponent* ASH_Waterfall2::GetMeshForType(ESH_MeshGenerationType MeshType)
{
	switch (MeshType)
	{
	case ESH_MeshGenerationType::W2_MG_Singular: return MeshComp_Singular;
	case ESH_MeshGenerationType::W2_MG_PerPath: return MeshComp_PerPath;
	case ESH_MeshGenerationType::W2_MG_Cross: return MeshComp_Cross;
	case ESH_MeshGenerationType::W2_MG_Splash: return MeshComp_Splash;
	}

	return nullptr;
}

#endif
TArray<ISH_WaterfallFxComponent*> ASH_Waterfall2::GetAllComponents_FX(bool bIncludeGlobals)
{
	TArray<ISH_WaterfallFxComponent*> Output = {};

	for (USH_WaterfallVFXComponent* ActorComp : GetAllComponents_VFX(bIncludeGlobals))
	{
		if (ISH_WaterfallFxComponent* FxComp = Cast<ISH_WaterfallFxComponent>(ActorComp)) Output.Add(FxComp);
	}
	for (USH_WaterfallSFXComponent* ActorComp : GetAllComponents_SFX(bIncludeGlobals))
	{
		if (ISH_WaterfallFxComponent* FxComp = Cast<ISH_WaterfallFxComponent>(ActorComp)) Output.Add(FxComp);
	}

	return Output;
}

TArray<USH_WaterfallVFXComponent*> ASH_Waterfall2::GetAllComponents_VFX(bool bIncludeGlobals)
{
	TArray<USH_WaterfallVFXComponent*> Output = {};
	GetComponents<USH_WaterfallVFXComponent>(Output);

	if (!bIncludeGlobals)
	{
		for (int i = Output.Num() - 1; i >= 0; i--)
		{
			if (Output[i] == VFX_Top
				|| Output[i] == VFX_Middle
				|| Output[i] == VFX_Bottom
				) Output.RemoveAt(i);
		}
	}

	return Output;
}

TArray<USH_WaterfallSFXComponent*> ASH_Waterfall2::GetAllComponents_SFX(bool bIncludeGlobals)
{
	TArray<USH_WaterfallSFXComponent*> Output = {};
	GetComponents<USH_WaterfallSFXComponent>(Output);

	if (!bIncludeGlobals)
	{
		for (int i = Output.Num() - 1; i >= 0; i--)
		{
			if (Output[i] == SFX_Top
				|| Output[i] == SFX_Middle
				|| Output[i] == SFX_Bottom
				) Output.RemoveAt(i);
		}
	}

	return Output;
}
#if WITH_EDITOR

TArray<ISH_WaterfallFxComponent*> ASH_Waterfall2::GetComponents_FX(int32 PathIndex, int32 PointIndex)
{
	TArray<ISH_WaterfallFxComponent*> OutComps = {};
	TArray<ISH_WaterfallFxComponent*> FxComps = GetAllComponents_FX();
	for (ISH_WaterfallFxComponent* FxComp : FxComps)
	{
		if (FxComp->PathIndex == PathIndex && FxComp->PointIndex == PointIndex)
		{
			OutComps.Add(FxComp);
		}
	}

	return OutComps;
}

void ASH_Waterfall2::DeleteComponent_FX(ISH_WaterfallFxComponent* CompToDelete, bool bRefreshDetailsPanel)
{
	GEngine->ForceGarbageCollection(true); //So that the names can be released before any other paths are created, in case GC is slow

	if (UActorComponent* DeletingComp = Cast<UActorComponent>(CompToDelete))
	{
		int32 PathIndex = CompToDelete->PathIndex;
		int32 PointIndex = CompToDelete->PointIndex;

		FString NewName = "DestroyedVFXComp " + DeletingComp->GetName();
		DeletingComp->Rename(*MakeUniqueObjectName(this, USH_WaterfallVFXComponent::StaticClass(), FName(NewName)).ToString(), DeletingComp->GetOuter());

		DeletingComp->DestroyComponent();

		MarkPackageDirty();

		if (GetComponents_FX(PathIndex, PointIndex).Num() <= 0) //If there are no more components on this path/point
		{
			USH_WaterfallPathComponent* TargetPath = GetPathComponent(PathIndex);
			if (TargetPath)
			{
				TargetPath->SetSimulatedPointHasFX(PointIndex, false);
			}
		}

		if (bRefreshDetailsPanel) RefreshDetailsPanel();
	}
}

void ASH_Waterfall2::DeleteAllComponents_FX(bool bRefreshDetailsPanel, bool bClearGlobals)
{
	GEngine->ForceGarbageCollection(true); //So that the names can be released before any other paths are created, in case GC is slow

	TArray<ISH_WaterfallFxComponent*> FxComps = GetAllComponents_FX();
	for (int i = FxComps.Num() - 1; i >= 0; i--)
	{
		//Theoretically this should only be called when regenerating the paths so the points will be deleted anyway, but just in case it's called elsewhere, set the bool
		USH_WaterfallPathComponent* TargetPath = GetPathComponent(FxComps[i]->PathIndex);
		if (TargetPath)
		{
			TargetPath->SetSimulatedPointHasFX(FxComps[i]->PointIndex, false);
		}

		if (UActorComponent* CompToKill = Cast<UActorComponent>(FxComps[i]))
		{
			CompToKill->DestroyComponent();
		}
	}

	if (bClearGlobals)
	{
		VFX_Top->ResetPoints();
		VFX_Middle->ResetPoints();
		VFX_Bottom->ResetPoints();
	}

	if (bRefreshDetailsPanel) RefreshDetailsPanel();
}

void ASH_Waterfall2::SetFxVisibility(bool bShowFx)
{
	for (USH_WaterfallVFXComponent* VfxComp : GetAllComponents_VFX(true))
	{
		if (VfxComp) VfxComp->SetVisibility(bShowFx);
	}
}

USH_WaterfallVFXComponent* ASH_Waterfall2::AddComponent_VFX(int32 PathIndex, int32 PointIndex, FTransform ComponentTransform, bool bRefreshDetailsPanel)
{
	FString NewName = "VFXComponent " + FString::SanitizeFloat(PathIndex, 0) + "_" + FString::SanitizeFloat(PointIndex, 0);
	if (USH_WaterfallVFXComponent* NewFxComp = NewObject<USH_WaterfallVFXComponent>(this, MakeUniqueObjectName(this, USH_WaterfallVFXComponent::StaticClass(), FName(NewName))))
	{
		NewFxComp->SetupAttachment(FxAttachPoint);
		AddInstanceComponent(NewFxComp);
		AddOwnedComponent(NewFxComp);
		NewFxComp->RegisterComponent();

		NewFxComp->SetWorldTransform(ComponentTransform);
		NewFxComp->PathIndex = PathIndex;
		NewFxComp->PointIndex = PointIndex;

		USH_WaterfallPathComponent* TargetPath = GetPathComponent(PathIndex);
		if (TargetPath)
		{
			TargetPath->SetSimulatedPointHasFX(PointIndex, true);
		}

		if (bRefreshDetailsPanel) RefreshDetailsPanel();

		MarkPackageDirty();

		return NewFxComp;
	}

	return nullptr;
}

USH_WaterfallSFXComponent* ASH_Waterfall2::AddComponent_SFX(int32 PathIndex, int32 PointIndex, FTransform ComponentTransform, bool bRefreshDetailsPanel)
{
	FString NewName = "SFXComponent " + FString::SanitizeFloat(PathIndex, 0) + "_" + FString::SanitizeFloat(PointIndex, 0);
	if (USH_WaterfallSFXComponent* NewFxComp = NewObject<USH_WaterfallSFXComponent>(this, MakeUniqueObjectName(this, USH_WaterfallSFXComponent::StaticClass(), FName(NewName))))
	{
		NewFxComp->SetupAttachment(FxAttachPoint);
		AddInstanceComponent(NewFxComp);
		AddOwnedComponent(NewFxComp);
		NewFxComp->RegisterComponent();

		NewFxComp->SetWorldTransform(ComponentTransform);
		NewFxComp->PathIndex = PathIndex;
		NewFxComp->PointIndex = PointIndex;

		USH_WaterfallPathComponent* TargetPath = GetPathComponent(PathIndex);
		if (TargetPath)
		{
			TargetPath->SetSimulatedPointHasFX(PointIndex, true);
		}

		if (bRefreshDetailsPanel) RefreshDetailsPanel();

		MarkPackageDirty();

		return NewFxComp;
	}

	return nullptr;
}

void ASH_Waterfall2::RefreshDetailsPanel()
{
	//Refresh the details panel (cheating by deselecting and then reselecting this)
	if (GEditor)
	{
		UEditorActorSubsystem* ActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		if (ActorSubsystem)
		{
			ActorSubsystem->SelectNothing();
			ActorSubsystem->SetSelectedLevelActors({ this });
		}
	}
}

bool ASH_Waterfall2::IsProcessing()
{
	return PathBuilder.IsProcessing() || MeshBuilder.IsProcessing() || StaticBuilder.IsProcessing();
}

TArray<FSH_VfxPointData> ASH_Waterfall2::GetAllStartVfxData()
{
	TArray<FSH_VfxPointData> Output = {};
	for (USH_WaterfallPathComponent* PathComponent : GetAllPathComponents())
	{
		if (PathComponent->GetNumberOfSplinePoints() > 0)
		{
			FVector ForwardVector = PathComponent->GetForwardVector();
			FVector RightVector = PathComponent->GetRightVector();

			//If the spline direction is flipped, the forward and right vectors need to be reversed
			if (GetWaterfallSettings()->bFlipSplineDirection)
			{
				ForwardVector *= -1.f;
				RightVector *= -1.f;
			}

			Output.Add(FSH_VfxPointData(
				PathComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World),
				ForwardVector,
				PathComponent->GetUpVector(),
				RightVector
			));
		}
	}

	return Output;
}

TArray<FSH_VfxPointData> ASH_Waterfall2::GetAllEndVfxData()
{
	TArray<FSH_VfxPointData> Output = {};
	for (USH_WaterfallPathComponent* PathComponent : GetAllPathComponents())
	{
		if (PathComponent->GetNumberOfSplinePoints() > 0)
		{
			Output.Add(FSH_VfxPointData(
				PathComponent->GetLocationAtSplinePoint(PathComponent->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World),
				PathComponent->GetDirectionAtSplinePoint(PathComponent->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World),
				PathComponent->GetUpVectorAtSplinePoint(PathComponent->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World) * -1,
				PathComponent->GetRightVectorAtSplinePoint(PathComponent->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World)
			));
		}
	}

	return Output;
}

TArray<FSH_VfxPointData> ASH_Waterfall2::GetAllVfxDataInRange(FVector2D DistanceRange, int32 RemapPoints)
{
	//Convert DistanceRange to world space z by finding the highest z point in a path and lowest z point in the path
	float HighestZ = 0.f;
	float LowestZ = 0.f;
	TArray<USH_WaterfallPathComponent*> Paths = GetAllPathComponents();
	for (int i = 0; i < Paths.Num(); i++)
	{
		if (Paths[i]->GetNumberOfSplinePoints() > 0)
		{
			if (i == 0)
			{
				HighestZ = Paths[i]->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World).Z;
				LowestZ = Paths[i]->GetLocationAtSplinePoint(Paths[i]->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World).Z;
			}

			for (int j = 0; j < Paths[i]->GetNumberOfSplinePoints(); j++)
			{
				float Location = Paths[i]->GetLocationAtSplinePoint(j, ESplineCoordinateSpace::World).Z;
				if (Location < LowestZ) LowestZ = Location;
				if (Location > HighestZ) HighestZ = Location;
			}
		}
	}

	float DistMin = LowestZ + ((HighestZ - LowestZ) * FMath::Min(DistanceRange.X, DistanceRange.Y));
	float DistMax = LowestZ + ((HighestZ - LowestZ) * FMath::Max(DistanceRange.X, DistanceRange.Y));

	TArray<FSH_VfxPointData> Output = {};
	for (USH_WaterfallPathComponent* PathComponent : Paths)
	{
		if (PathComponent->GetNumberOfSplinePoints() > 0)
		{
			//1. There is no point in doing any of this if the first and last points are outside the range
			FVector StartPoint = PathComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
			FVector EndPoint = PathComponent->GetLocationAtSplinePoint(PathComponent->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);
			if (StartPoint.Z < DistMin || EndPoint.Z > DistMax) continue;

			//2. Find the spline points that encompass the Range Points
			int32 IndexA = 0;
			int32 IndexB = 0;
			int32 IndexC = 0;
			int32 IndexD = 0;
			for (int i = 1; i <= PathComponent->GetNumberOfSplinePoints(); i++)
			{
				FVector CurrentPoint = PathComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
				if (CurrentPoint.Z > DistMax) IndexA = i;
				else if (CurrentPoint.Z <= DistMax && IndexB == 0) IndexB = i;
				else if (CurrentPoint.Z > DistMin) IndexC = i;
				else if (CurrentPoint.Z <= DistMin)
				{
					IndexD = i;
					break;
				}
			}

			//If the last point is in the distance range it needs to be set because the above function will not set it
			if (IndexD == 0) IndexD = PathComponent->GetNumberOfSplinePoints() - 1;

			//3. Find the alpha between the two spline points for the start and end
			//Note: This won't be 100% accurate but it will get very close, too close to notice that it's not quite accurate
			//It will be less accurate on splines that have stronger arrive and leave tangents
			//However, in the majority of use-cases of waterfall generation for this tool, this calculation should be sufficient enough
			float AlphaStart = 0.f;
			float AlphaEnd = 0.f;
			if (IndexA != IndexB)
			{
				FVector PointA = PathComponent->GetLocationAtSplinePoint(IndexA, ESplineCoordinateSpace::World);
				FVector PointB = PathComponent->GetLocationAtSplinePoint(IndexB, ESplineCoordinateSpace::World);
				float A = PointA.Z;
				float B = PointB.Z - A;
				float C = DistMax - A;
				AlphaStart = C / B;
				//This provides a percentage between 0 - 1 of the distance from PointA.Z to PointB.Z of where DistMax sits
			}

			if (IndexC != IndexD)
			{
				FVector PointC = PathComponent->GetLocationAtSplinePoint(IndexC, ESplineCoordinateSpace::World);
				FVector PointD = PathComponent->GetLocationAtSplinePoint(IndexD, ESplineCoordinateSpace::World);
				float A = PointC.Z;
				float B = PointD.Z - A;
				float C = DistMin - A;
				AlphaEnd = C / B;
				//This provides a percentage between 0 - 1 of the distance from PointC.Z to PointD.Z of where DistMax sits
			}

			//4. Find the time along the spline at IndexA + AlphaStart and IndexC + AlphaEnd both converted from segment percentages to overall spline percentages
			float TimeA = PathComponent->GetTimeAtDistanceAlongSpline(PathComponent->GetDistanceAlongSplineAtSplinePoint(IndexA));
			float TimeB = PathComponent->GetTimeAtDistanceAlongSpline(PathComponent->GetDistanceAlongSplineAtSplinePoint(IndexB));
			float TimeC = PathComponent->GetTimeAtDistanceAlongSpline(PathComponent->GetDistanceAlongSplineAtSplinePoint(IndexC));
			float TimeD = PathComponent->GetTimeAtDistanceAlongSpline(PathComponent->GetDistanceAlongSplineAtSplinePoint(IndexD));

			float TimeStart = TimeA + (AlphaStart * (TimeB - TimeA));
			float TimeEnd = TimeC + (AlphaEnd * (TimeD - TimeC));

			//Get the remapped output locations
			Output.Add(FSH_VfxPointData(
				PathComponent->GetLocationAtTime(TimeStart, ESplineCoordinateSpace::World),
				PathComponent->GetDirectionAtTime(TimeStart, ESplineCoordinateSpace::World),
				PathComponent->GetUpVectorAtTime(TimeStart, ESplineCoordinateSpace::World) * -1,
				PathComponent->GetRightVectorAtTime(TimeStart, ESplineCoordinateSpace::World)
			));

			for (int i = 1; i <= RemapPoints - 2; i++)
			{
				float Alpha = (TimeEnd - TimeStart) / (RemapPoints - 1);
				float TimeMid = TimeStart + (Alpha * i);
				Output.Add(FSH_VfxPointData(
					PathComponent->GetLocationAtTime(TimeMid, ESplineCoordinateSpace::World),
					PathComponent->GetDirectionAtTime(TimeMid, ESplineCoordinateSpace::World),
					PathComponent->GetUpVectorAtTime(TimeMid, ESplineCoordinateSpace::World) * -1,
					PathComponent->GetRightVectorAtTime(TimeMid, ESplineCoordinateSpace::World)
				));
			}

			Output.Add(FSH_VfxPointData(
				PathComponent->GetLocationAtTime(TimeEnd, ESplineCoordinateSpace::World),
				PathComponent->GetDirectionAtTime(TimeEnd, ESplineCoordinateSpace::World),
				PathComponent->GetUpVectorAtTime(TimeEnd, ESplineCoordinateSpace::World) * -1,
				PathComponent->GetRightVectorAtTime(TimeEnd, ESplineCoordinateSpace::World)
			));
		}
	}

	return Output;
}

TArray<FSH_VfxPointData> ASH_Waterfall2::GetAllVfxDataInRange(FIntVector2 PointRange)
{
	TArray<FSH_VfxPointData> Output = {};
	for (USH_WaterfallPathComponent* PathComponent : GetAllPathComponents())
	{
		if (PathComponent->GetNumberOfSplinePoints() > 0)
		{
			for (int i = FMath::Min(PointRange.X, PointRange.Y); i <= FMath::Max(PointRange.X, PointRange.Y); i++)
			{
				if (i > 0 && i < PathComponent->GetNumberOfSplinePoints())
				{
					Output.Add(FSH_VfxPointData(
						PathComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World),
						PathComponent->GetDirectionAtSplinePoint(i, ESplineCoordinateSpace::World)
					));
				}
			}
		}
	}

	return Output;
}

#endif
void ASH_Waterfall2::OnStaticMeshPropChange()
{
	//Make sure we show or hide the static mesh in game based on whether it's nullptr or not
	bShowDynamicMeshesInGame = (BakedMeshComp) ? BakedMeshComp->GetStaticMesh() == nullptr : true;
}

#if WITH_EDITOR
void ASH_Waterfall2::PostLoad()
{
	Super::PostLoad();
	if (GetWorld() && GetWorld()->IsEditorWorld())
	{
		// Use a short delay to ensure everything is ready for the "cold start" editor load.
		FTimerHandle UnusedHandle;
		GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &ASH_Waterfall2::DeferredVfxUpdate, 0.1f, false);
	}
}

void ASH_Waterfall2::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Using a timer to defer the update is the most robust way to handle editor initialization.
	// This ensures that the world is fully loaded and ready before we try to update the components.
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASH_Waterfall2::DeferredVfxUpdate);
	}
}

void ASH_Waterfall2::DeferredVfxUpdate()
{
	TArray<USH_WaterfallVFXComponent*> VfxComps;
	GetComponents<USH_WaterfallVFXComponent>(VfxComps);
	for (USH_WaterfallVFXComponent* VfxComp : VfxComps)
	{
		if (VfxComp)
		{
			VfxComp->UpdateComponentParams();
		}
	}
}
#endif //WITH_EDITOR

