// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Settings/SH_Waterfall2Settings.h"

void USH_Waterfall2Settings::UpdateAutoFocusWaterfall(ECheckBoxState NewCheckboxState)
{
	bAutoFocusWaterfallOnSelection = NewCheckboxState == ECheckBoxState::Checked;

	SaveConfig();
}