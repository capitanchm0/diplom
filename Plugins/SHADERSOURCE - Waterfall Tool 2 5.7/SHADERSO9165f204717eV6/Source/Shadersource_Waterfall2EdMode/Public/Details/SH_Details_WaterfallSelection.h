// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/*Normally these includes would JUST be in the .cpp files of Details Customisations
* but this will just add it by default when this header is added*/
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "IPropertyUtilities.h"
#include "IDetailGroup.h"
//--------------------------------------

/* Details Customisation for the selection actor
* (shows up in the Editor Mode when there is no Waterfall actor selected). */
class SHADERSOURCE_WATERFALL2EDMODE_API FSH_Details_WaterfallSelection : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShareable(new FSH_Details_WaterfallSelection); }

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) final;

private:
	//bool OnShouldFilterOutAsset_CreateNew(const FAssetData& AssetData);

	//Whether to auto focus the waterfall when it's selected via the UI
	TSharedPtr<SCheckBox> CHB_AutoFocusWaterfall;

public:
	static FReply ButtonClicked_SelectActor(AActor* ActorToSelect);
	static void SelectActor(AActor* ActorToSelect);
	static TSharedPtr<SWidget> CreateAutoFocusCheckbox(TSharedPtr<SCheckBox>& CheckboxToAssign, FMargin Padding = FMargin(0.f));
};
