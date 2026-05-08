// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"
#include "EditorComponents/SH_WaterfallGenerationEnums.h"
#include "SH_WaterfallMeshComponent.generated.h"

/* A Dynamic Mesh Component used for generating the meshes in the Editor Mode.
* Currently this is not avaialble at runtime. */
UCLASS()
class SHADERSOURCE_WATERFALL2RUNTIME_API USH_WaterfallMeshComponent : public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waterfall")
		ESH_MeshGenerationType MeshType = ESH_MeshGenerationType::W2_MG_PerPath;
};
