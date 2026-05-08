// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Generation/SH_WaterfallBuilder.h"

/* Builder to convert the dynamic mesh data into static mesh assets. */
struct SHADERSOURCE_WATERFALL2RUNTIME_API FSH_WaterfallBuilder_Static : public FSH_WaterfallBuilder
{
	friend class ASH_Waterfall2;

public:
	FSH_WaterfallBuilder_Static() {}
	FSH_WaterfallBuilder_Static(ASH_Waterfall2* _ParentWaterfall)
		: FSH_WaterfallBuilder(_ParentWaterfall)
	{

	}

#if WITH_EDITOR
	//Bake to Static Mesh
public:
	void BakeToStaticMesh(FSimpleDelegate OnProcessingFinished = FSimpleDelegate());
#endif
};
