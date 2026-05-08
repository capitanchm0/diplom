// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Generation/SH_WaterfallBuilder.h"
#include "Actors/SH_Waterfall2.h"

#if WITH_EDITOR
FSH_WaterfallBuilder::FSH_WaterfallBuilder(ASH_Waterfall2* _ParentWaterfall)
{
	ParentWaterfall = _ParentWaterfall;
}

USH_WaterfallSettingsComponent* FSH_WaterfallBuilder::GetWaterfallSettings()
{
	if (!SettingsComponent)
	{
		if (GetParentWaterfall())
		{
			SettingsComponent = GetParentWaterfall()->GetWaterfallSettings();
		}
	}

	return SettingsComponent;
}

void FSH_WaterfallBuilder::StartProcessing()
{
	NumPathsProcessed = 0;
	SetIsProcessing(true);
}

void FSH_WaterfallBuilder::ProcessingFinished()
{
	SetIsProcessing(false);
	NumPathsProcessed = 0;

	PathsToProcess.Empty();
	PathsProcessing.Empty();
}
#endif