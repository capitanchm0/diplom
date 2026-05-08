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

/* Details customisation for the Waterfall Mesh Component */
class SHADERSOURCE_WATERFALL2EDMODE_API FSH_Details_WaterfallMeshComp : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShareable(new FSH_Details_WaterfallMeshComp); }

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) final;
};
