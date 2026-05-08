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

/* This is to hide and reorder the Details of the whole Waterfall Actor when selected in the main Details Panel outside of the Ed Mode. */
class SHADERSOURCE_WATERFALL2EDMODE_API SH_Details_Waterfall2 : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShareable(new SH_Details_Waterfall2); }

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) final;
};
