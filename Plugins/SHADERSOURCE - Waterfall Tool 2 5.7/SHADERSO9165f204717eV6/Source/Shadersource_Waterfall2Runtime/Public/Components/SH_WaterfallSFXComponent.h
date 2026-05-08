// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "Components/ISH_WaterfallFxComponent.h"
#include "SH_WaterfallSFXComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (SHADERSOURCE), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "SH Waterfall SFX Component"))
class SHADERSOURCE_WATERFALL2RUNTIME_API USH_WaterfallSFXComponent : public UAudioComponent, public ISH_WaterfallFxComponent
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Waterfall")
		int32 GetPathIndex() { return PathIndex; }
	UFUNCTION(BlueprintPure, Category = "Waterfall")
		int32 GetPointIndex() { return PointIndex; }
};
