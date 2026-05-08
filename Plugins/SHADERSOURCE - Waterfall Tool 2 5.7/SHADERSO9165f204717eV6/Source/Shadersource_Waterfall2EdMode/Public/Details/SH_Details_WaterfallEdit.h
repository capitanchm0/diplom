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

/* Details customisation for the Waterfall Settings Component
* (visualised in the Editor Mode when a Waterfall actor is selected). */
class SHADERSOURCE_WATERFALL2EDMODE_API FSH_Details_WaterfallEdit : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShareable(new FSH_Details_WaterfallEdit); }

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) final;

private:
	TSharedPtr<class IPropertyUtilities> PropUtils;
	class USH_WaterfallSettingsComponent* Settings = nullptr;

	bool IsEnabled_FxTabContent() const;

	void RefreshSlate();

	class USplineComponent* SplineComp_TopSpline;
	TSharedPtr<class FSplineComponentVisualizer> SplineVisualizer;
	int32 Index = INDEX_NONE;

	void GenerateSplinePointSelectionControls(IDetailCategoryBuilder& ChildrenBuilder);
	FReply OnSelectFirstLastSplinePoint(bool bFirst);
	FReply OnSelectPrevNextSplinePoint(bool bNext, bool bAddToSelection);
	FReply OnSelectAllSplinePoints();
	//bool IsOnePointSelected() const;
	bool ArePointsSelected() const;

	FReply ButtonClicked_FocusComponent(USceneComponent* Component);
};
