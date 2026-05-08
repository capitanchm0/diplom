// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Details/SH_Details_WaterfallMeshComp.h"

void FSH_Details_WaterfallMeshComp::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.HideCategory("Transform");
	DetailBuilder.HideCategory("Materials");
	DetailBuilder.HideCategory("Physics");
	DetailBuilder.HideCategory("BodySetup");
	DetailBuilder.HideCategory("HLOD");
	DetailBuilder.HideCategory("VirtualTexture");
	DetailBuilder.HideCategory("Activation");
	DetailBuilder.HideCategory("Cooking");
}