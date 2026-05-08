// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Components/ISH_WaterfallFxComponent.h"
#include "Components/SceneComponent.h"

USceneComponent* ISH_WaterfallFxComponent::AsComponent()
{
	return Cast<USceneComponent>(this);
}