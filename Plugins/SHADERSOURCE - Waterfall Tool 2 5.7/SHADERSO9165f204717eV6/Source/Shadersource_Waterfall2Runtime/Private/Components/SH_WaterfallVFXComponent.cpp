// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Components/SH_WaterfallVFXComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Actors/SH_Waterfall2.h"


USH_WaterfallVFXComponent::USH_WaterfallVFXComponent()
{
	//This component doesn't need to tick
	PrimaryComponentTick.bCanEverTick = false;

#if WITH_EDITOR
	//Set VFX bounds on instance change
	OnSystemInstanceChanged().AddUObject(this, &USH_WaterfallVFXComponent::SetVFXBounds);
#endif //WITH_EDITOR
}

void USH_WaterfallVFXComponent::SetVFXBounds()
{
	//Set the VFX Fixed Bounds
	SetSystemFixedBounds(VFXBoundsBox);
}

TArray<FVector> ConvToLocal(FTransform T, TArray<FVector> Input)
{
	TArray<FVector> Output = {};
	for (const FVector& InputVec : Input)
	{
		Output.Add(T.InverseTransformPosition(InputVec));
	}

	return Output;
}

TArray<FVector> USH_WaterfallVFXComponent::RotateToActor(TArray<FVector> Input)
{
	TArray<FVector> Output = {};
	for (const FVector& InputVec : Input)
	{
		Output.Add(InputVec.RotateAngleAxis(GetOwner()->GetActorRotation().Yaw, FVector(0, 0, -1)));
	}

	return Output;
}

TArray<FVector> USH_WaterfallVFXComponent::GetPositions()
{
	TArray<FVector> OutVector = {};
	for (const FSH_VfxPointData& Data : PointData)
	{
		OutVector.Add(Data.Position);
	}

	return OutVector;
}

TArray<FVector> USH_WaterfallVFXComponent::GetForwardDirections()
{
	TArray<FVector> OutVector = {};
	for (const FSH_VfxPointData& Data : PointData)
	{
		OutVector.Add(Data.ForwardDirection);
	}

	return OutVector;
}
TArray<FVector> USH_WaterfallVFXComponent::GetUpDirections()
{
	TArray<FVector> OutVector = {};
	for (const FSH_VfxPointData& Data : PointData)
	{
		OutVector.Add(Data.UpDirection);
	}

	return OutVector;
}
TArray<FVector> USH_WaterfallVFXComponent::GetRightDirections()
{
	TArray<FVector> OutVector = {};
	for (const FSH_VfxPointData& Data : PointData)
	{
		OutVector.Add(Data.RightDirection);
	}

	return OutVector;
}

void USH_WaterfallVFXComponent::ResetPoints()
{
	PointData.Empty();
	UpdateComponentParams();
}

void USH_WaterfallVFXComponent::UpdateComponentParams()
{
	// Don't run this on the Class Default Object
	if (IsTemplate())
	{
		return;
	}

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(this, "PositionArray", ConvToLocal(GetComponentTransform(), GetPositions()));
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(this, "ForwardDirectionArray", RotateToActor(GetForwardDirections()));
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(this, "UpDirectionArray", RotateToActor(GetUpDirections()));
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(this, "RightDirectionArray", RotateToActor(GetRightDirections()));

	//Set the VFX fixed bounds to the actor bounds
	if (GetOwner())
	{
		FBox ActorBoxBounds = GetOwner()->CalculateComponentsBoundingBoxInLocalSpace();
		VFXBoundsBox = ActorBoxBounds;
		SetVFXBounds();
	}

	if (GetAsset())
	{
		if (IsActive())
		{
			Deactivate();
		}
		Activate(true);
	}

//#if WITH_EDITOR
//	MarkPackageDirty();
//#endif //WITH_EDITOR
}

#if WITH_EDITOR
void USH_WaterfallVFXComponent::DrawPoints()
{
	for (const FSH_VfxPointData& Data : PointData)
	{
		DrawDebugPoint(GetWorld(), Data.Position, 20.f, FColor::Green, false, 5.f);
	}
}
#endif //WITH_EDITOR

