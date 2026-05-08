// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ISH_WaterfallFxComponent.generated.h"

/**
 * 
 */
UINTERFACE(NotBlueprintable)
class SHADERSOURCE_WATERFALL2RUNTIME_API USH_WaterfallFxComponent : public UInterface
{
	GENERATED_BODY()
};

class SHADERSOURCE_WATERFALL2RUNTIME_API ISH_WaterfallFxComponent
{
	GENERATED_BODY()

public:
	class USceneComponent* AsComponent();

	//Which path this component belongs to
	int32 PathIndex = INDEX_NONE;
	//Which point on the path this component belongs to
	int32 PointIndex = INDEX_NONE;

	virtual void UpdateComponentParams() {};
};
