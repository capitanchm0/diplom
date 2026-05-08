// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SH_Waterfall2Settings.generated.h"

/* Waterfall Tool 2 settings for display in Project Settings. */
UCLASS(config = SHADERSOURCEPerProjectSettings, meta = (DisplayName = "SHADERSOURCE: Waterfall Tool 2 Settings"))
class SHADERSOURCE_WATERFALL2EDMODE_API USH_Waterfall2Settings : public UDeveloperSettings
{
	GENERATED_BODY()
	
protected:
	virtual FName GetCategoryName() const { return FName("Plugins"); }

public:
	UPROPERTY(EditAnywhere, config, Category = "Selection")
		bool bAutoFocusWaterfallOnSelection = false;

	void UpdateAutoFocusWaterfall(ECheckBoxState NewCheckboxState);
};
