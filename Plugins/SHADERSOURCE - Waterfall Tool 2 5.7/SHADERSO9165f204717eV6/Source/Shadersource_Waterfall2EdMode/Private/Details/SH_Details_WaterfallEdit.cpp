// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Details/SH_Details_WaterfallEdit.h"
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "Components/SH_WaterfallSFXComponent.h"
#include "Components/SH_WaterfallVFXComponent.h"
#include "Actors/SH_Waterfall2.h"
#include "SH_WaterfallTool2Statics.h"
#include "Slate/SSH_FxDisplay.h"

#include "Subsystems/EditorSubsystemBlueprintLibrary.h"
#include "SplineComponentVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "LevelEditorViewport.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Engine/Selection.h"

void FSH_Details_WaterfallEdit::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	PropUtils = DetailBuilder.GetPropertyUtilities();

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomised;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomised);

	if (ObjectsBeingCustomised.Num() > 0)
	{
		Settings = Cast<USH_WaterfallSettingsComponent>(ObjectsBeingCustomised[0]);
		if (Settings)
		{
			DetailBuilder.HideCategory("Tags");
			DetailBuilder.HideCategory("ComponentReplication");
			DetailBuilder.HideCategory("Activation");
			DetailBuilder.HideCategory("Variable");
			DetailBuilder.HideCategory("Cooking");
			DetailBuilder.HideCategory("AssetUserData");
			DetailBuilder.HideCategory("Replication");
			DetailBuilder.HideCategory("Collision");
			DetailBuilder.HideCategory("Navigation");

			auto CombineTwoPropertyRows = [&](IDetailCategoryBuilder& Category, TSharedPtr<IPropertyHandle> PropA, TSharedPtr<IPropertyHandle> PropB)
			{
				DetailBuilder.HideProperty(PropA);
				DetailBuilder.HideProperty(PropB);

				Category.AddCustomRow(PropA->GetPropertyDisplayName())
					.NameContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(0.f, 0.f, 4.f, 0.f)
						[
							PropA->CreatePropertyValueWidget()
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							PropA->CreatePropertyNameWidget()
						]
					]
					.ValueContent()
					[
						PropB->CreatePropertyValueWidget()
					];
			};

			auto GenerateRandomSeedRow = [&](IDetailCategoryBuilder& Category)
			{
				TSharedPtr<IPropertyHandle> Prop_Seed = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, Seed));
				DetailBuilder.HideProperty(Prop_Seed);
				Category.AddCustomRow(FText::FromString("Seed"))
					.NameContent()
					[
						Prop_Seed->CreatePropertyNameWidget()
					]
					.ValueContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(EHorizontalAlignment::HAlign_Left)
						.Padding(0.f, 0.f, 2.f, 0.f)
						[
							SNew(SButton)
							.ContentPadding(0.f)
							.ToolTipText(FText::FromString("Randomise the seed."))
							.OnClicked(FOnClicked::CreateUObject(Settings, &USH_WaterfallSettingsComponent::ButtonClicked_RandomiseSeed))
							[
								SNew(SBox) //Default icon size for a Details Panel Row
								.WidthOverride(12.f)
								.HeightOverride(12.f)
								[
									SNew(SScaleBox)
									.Stretch(EStretch::ScaleToFit)
									[
										SNew(SImage)
										.Image(FAppStyle::Get().GetBrush("Icons.Refresh"))
									]
								]
							]
						]
						+ SHorizontalBox::Slot()
						[
							Prop_Seed->CreatePropertyValueWidget()
						]
					];
			};

			auto AddKillPlaneToCategory = [&](IDetailCategoryBuilder& Category)
			{
				UStaticMeshComponent* KillPlane = Settings->GetParentWaterfall()->GetKillPlane();
				IDetailPropertyRow* Row = Category.AddExternalObjectProperty({ KillPlane }, USceneComponent::GetRelativeLocationPropertyName());
				if (Row)
				{
					Row->DisplayName(FText::FromString("Kill Plane Location"));
					Row->ToolTip(FText::FromString("The X-Y values are purely cosmetic. The Z value affects where the waterfall paths will end."));
				}
			};

			auto AddSplineSamplesToCategory = [&](IDetailCategoryBuilder& Category)
			{
				TSharedPtr<IPropertyHandle> Prop_SplineSample = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bResampleSpline));
				TSharedPtr<IPropertyHandle> Prop_NumSplinePoints = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, NumSplinePoints));

				DetailBuilder.HideProperty(Prop_SplineSample);
				DetailBuilder.HideProperty(Prop_NumSplinePoints);

				Category.AddCustomRow(Prop_NumSplinePoints->GetPropertyDisplayName())
					.NameContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(EHorizontalAlignment::HAlign_Left)
						.Padding(0.f, 0.f, 3.f, 0.f)
						[
							Prop_SplineSample->CreatePropertyValueWidget(false)
						]
						+ SHorizontalBox::Slot()
						[
							Prop_NumSplinePoints->CreatePropertyNameWidget()
						]
					]
					.ValueContent()
					[
						Prop_NumSplinePoints->CreatePropertyValueWidget()
					];
			};

			if (Settings->CurrentTabState == ESH_W2_TabState::W2_TS_Simple)
			{
				IDetailCategoryBuilder& Cat_PathSettings = DetailBuilder.EditCategory("PathSettings", FText::GetEmpty(), ECategoryPriority::Default);
				//This UI doesn't work in 5.4, so it's commented out for now
				//GenerateSplinePointSelectionControls(Cat_PathSettings);
				Cat_PathSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, NumPaths));
				Cat_PathSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bAddSpawnJitter));
				Cat_PathSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SpawnJitterRange));
				//This should only be visible if there are more than one path
				if (Settings->NumPaths > 1) Cat_PathSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, PathSpawnRange));
				Cat_PathSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, Gravity));
				AddSplineSamplesToCategory(Cat_PathSettings);
				AddKillPlaneToCategory(Cat_PathSettings);
				Cat_PathSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, KillPlaneScaleOffset));
				GenerateRandomSeedRow(Cat_PathSettings);
				Cat_PathSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, IgnoreActorsForAvoidance));

				IDetailCategoryBuilder& Cat_MeshSettings = DetailBuilder.EditCategory("MeshGenerationSettings", FText::GetEmpty(), ECategoryPriority::Default);

				auto ProcessSimpleBoolProp = [&](const FName BoolPropertyPath, const FName MaterialPropertyPath) -> TSharedPtr<SWidget>
				{
					TSharedPtr<IPropertyHandle> Prop_Bool = DetailBuilder.GetProperty(BoolPropertyPath);
					TSharedPtr<IPropertyHandle> Prop_Mat = DetailBuilder.GetProperty(MaterialPropertyPath);
					DetailBuilder.HideProperty(Prop_Bool);

					bool ResolvedBool = false;
					Prop_Bool->GetValue(ResolvedBool);

					return SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							[
								Prop_Bool->CreatePropertyNameWidget()
							]
							+ SHorizontalBox::Slot()
							[
								Prop_Bool->CreatePropertyValueWidget()
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBox)
							.Visibility(ResolvedBool ? EVisibility::Visible : EVisibility::Collapsed)
							[
								Prop_Mat->CreatePropertyValueWidget()
							]
						];
				};

				Cat_MeshSettings.AddCustomRow(FText::FromString("MeshSelection"))
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.f, 0.f, 0.f, 2.f)
							[
								SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.Padding(0.f, 0.f, 2.f, 0.f)
									[
										ProcessSimpleBoolProp(
											GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SimpleGenerateSingleMesh),
											GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SingularMeshMaterial)
										).ToSharedRef()
									]
									+ SHorizontalBox::Slot()
									[
										ProcessSimpleBoolProp(
											GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SimpleGeneratePerPathMesh),
											GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, PerPathMeshMaterial)
										).ToSharedRef()
									]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.Padding(0.f, 0.f, 2.f, 0.f)
									[
										ProcessSimpleBoolProp(
											GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SimpleGenerateSplashMesh),
											GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SplashMeshMaterial)
										).ToSharedRef()
									]
									+ SHorizontalBox::Slot()
									[
										ProcessSimpleBoolProp(
											GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SimpleGeneratePlaneMesh),
											GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, CrossMeshMaterial)
										).ToSharedRef()
									]
							]
					];

				IDetailCategoryBuilder& Cat_SFX = DetailBuilder.EditCategory("SFX", FText::GetEmpty(), ECategoryPriority::Default);
				Cat_SFX.AddExternalObjectProperty({ Settings->GetParentWaterfall()->GetSFX_Top() }, GET_MEMBER_NAME_CHECKED(USH_WaterfallSFXComponent, Sound))->DisplayName(FText::FromString("Top SFX"));
				Cat_SFX.AddExternalObjectProperty({ Settings->GetParentWaterfall()->GetSFX_Middle() }, GET_MEMBER_NAME_CHECKED(USH_WaterfallSFXComponent, Sound))->DisplayName(FText::FromString("Middle SFX"));
				Cat_SFX.AddExternalObjectProperty({ Settings->GetParentWaterfall()->GetSFX_Bottom() }, GET_MEMBER_NAME_CHECKED(USH_WaterfallSFXComponent, Sound))->DisplayName(FText::FromString("Bottom SFX"));

				IDetailCategoryBuilder& Cat_VFxSettings = DetailBuilder.EditCategory("VFX Settings", FText::GetEmpty(), ECategoryPriority::Default);
				CombineTwoPropertyRows(Cat_VFxSettings, DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bTopVFX)), DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, TopVfxSystem)));
				CombineTwoPropertyRows(Cat_VFxSettings, DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bMiddleVFX)), DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, MiddleVfxSystem)));
				CombineTwoPropertyRows(Cat_VFxSettings, DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bBottomVFX)), DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, BottomVfxSystem)));

				IDetailCategoryBuilder& Cat_BakeMeshSettings = DetailBuilder.EditCategory("BakeMeshSettings", FText::GetEmpty(), ECategoryPriority::Default);
			}
			else
			{
				DetailBuilder.HideCategory("PathSettings");
				DetailBuilder.HideCategory("MeshGenerationSettings");
				DetailBuilder.HideCategory("SFX");
				DetailBuilder.HideCategory("VFX Settings");
				DetailBuilder.HideCategory("BakeMeshSettings");
			}

			if (Settings->CurrentTabState == ESH_W2_TabState::W2_TS_Paths)
			{
				//This UI doesn't work in 5.4, so it's commented out for now
				//IDetailCategoryBuilder& Cat_TopSpline = DetailBuilder.EditCategory("TopSpline", FText::GetEmpty(), ECategoryPriority::Default);
				//GenerateSplinePointSelectionControls(Cat_TopSpline);
				IDetailCategoryBuilder& Cat_InitialSpawn = DetailBuilder.EditCategory("InitialSpawn", FText::GetEmpty(), ECategoryPriority::Default);
				Cat_InitialSpawn.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, NumPaths));
				//This should only be visible if there are more than one path
				if (Settings->NumPaths > 1)
				{
					Cat_InitialSpawn.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, PathSpawnRange));
					DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SinglePathPosition));
				}
				else
				{
					Cat_InitialSpawn.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, SinglePathPosition));
					DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, PathSpawnRange));
				}

				IDetailCategoryBuilder& Cat_Simulation = DetailBuilder.EditCategory("Simulation", FText::GetEmpty(), ECategoryPriority::Default);
				AddSplineSamplesToCategory(Cat_Simulation);
				IDetailCategoryBuilder& Cat_Random = DetailBuilder.EditCategory("Random", FText::GetEmpty(), ECategoryPriority::Default);
				GenerateRandomSeedRow(Cat_Random);
				IDetailCategoryBuilder& Cat_Kill = DetailBuilder.EditCategory("Kill", FText::GetEmpty(), ECategoryPriority::Default);
				IDetailCategoryBuilder& Cat_Speed = DetailBuilder.EditCategory("Speed", FText::GetEmpty(), ECategoryPriority::Default);
				IDetailCategoryBuilder& Cat_Direction = DetailBuilder.EditCategory("Direction", FText::GetEmpty(), ECategoryPriority::Default);
				IDetailCategoryBuilder& Cat_Physics = DetailBuilder.EditCategory("Physics", FText::GetEmpty(), ECategoryPriority::Default);
				IDetailCategoryBuilder& Cat_Avoidance = DetailBuilder.EditCategory("Avoidance", FText::GetEmpty(), ECategoryPriority::Default);

				//This category is hidden for now as modifying the values can give odd results
				//IDetailCategoryBuilder& Cat_Flow = DetailBuilder.EditCategory("Flow", FText::GetEmpty(), ECategoryPriority::Default);
				DetailBuilder.HideCategory("Flow");

				AddKillPlaneToCategory(Cat_Kill);
			}
			else
			{
				DetailBuilder.HideCategory("InitialSpawn");
				DetailBuilder.HideCategory("Simulation");
				DetailBuilder.HideCategory("Kill");
				DetailBuilder.HideCategory("Speed");
				DetailBuilder.HideCategory("Direction");
				DetailBuilder.HideCategory("Physics");
				DetailBuilder.HideCategory("Avoidance");
				DetailBuilder.HideCategory("Flow");
				DetailBuilder.HideCategory("Random");
			}

			if (Settings->CurrentTabState == ESH_W2_TabState::W2_TS_Meshes)
			{
				IDetailCategoryBuilder& Cat_MeshSelection =			DetailBuilder.EditCategory("MeshSelection",			FText::GetEmpty(), ECategoryPriority::Transform);
				IDetailCategoryBuilder& Cat_GenerationSettings =	DetailBuilder.EditCategory("GenerationSettings",	FText::GetEmpty(), ECategoryPriority::Default);
				IDetailCategoryBuilder& Cat_MeshSettings =			DetailBuilder.EditCategory("MeshSettings",			FText::GetEmpty(), ECategoryPriority::Default);

				//Show or hide categories depending on the current mesh that we're working on
				if (Settings->SelectedMesh == ESH_MeshGenerationType::W2_MG_Singular)
				{
					IDetailCategoryBuilder& Cat_SingularMeshSettings = DetailBuilder.EditCategory("SingularMeshSettings", FText::GetEmpty(), ECategoryPriority::Default);
					DetailBuilder.HideCategory("Bulge");
				}
				else DetailBuilder.HideCategory("SingularMeshSettings");
				if (Settings->SelectedMesh == ESH_MeshGenerationType::W2_MG_PerPath) IDetailCategoryBuilder& Cat_PerPathSettings = DetailBuilder.EditCategory("PerPathSettings", FText::GetEmpty(), ECategoryPriority::Default);
				else DetailBuilder.HideCategory("PerPathSettings");
				if (Settings->SelectedMesh == ESH_MeshGenerationType::W2_MG_Cross) IDetailCategoryBuilder& Cat_PlaneMeshSettings = DetailBuilder.EditCategory("PlaneMeshSettings", FText::GetEmpty(), ECategoryPriority::Default);
				else DetailBuilder.HideCategory("PlaneMeshSettings");
				if (Settings->SelectedMesh == ESH_MeshGenerationType::W2_MG_Splash) IDetailCategoryBuilder& Cat_SplashMeshSettings = DetailBuilder.EditCategory("SplashMeshSettings", FText::GetEmpty(), ECategoryPriority::Default);
				else DetailBuilder.HideCategory("SplashMeshSettings");

				IDetailCategoryBuilder& Cat_Positions =		DetailBuilder.EditCategory("Positions",		FText::GetEmpty(), ECategoryPriority::Default);
				IDetailCategoryBuilder& Cat_Normals =		DetailBuilder.EditCategory("Normals",		FText::GetEmpty(), ECategoryPriority::Default);
				IDetailCategoryBuilder& Cat_Tangents =		DetailBuilder.EditCategory("Tangents",		FText::GetEmpty(), ECategoryPriority::Default);
				IDetailCategoryBuilder& Cat_UVs =			DetailBuilder.EditCategory("UVs",			FText::GetEmpty(), ECategoryPriority::Default);
				IDetailCategoryBuilder& Cat_Turbulence =	DetailBuilder.EditCategory("Turbulence",	FText::GetEmpty(), ECategoryPriority::Default);

				if (Settings->SelectedMesh == ESH_MeshGenerationType::W2_MG_PerPath
					|| Settings->SelectedMesh == ESH_MeshGenerationType::W2_MG_Cross
					|| Settings->SelectedMesh == ESH_MeshGenerationType::W2_MG_Splash)
				{
					IDetailCategoryBuilder& Cat_Bulge = DetailBuilder.EditCategory("Bulge", FText::GetEmpty(), ECategoryPriority::Default);
				}
			}
			else
			{
				DetailBuilder.HideCategory("MeshSelection");
				DetailBuilder.HideCategory("GenerationSettings");
				DetailBuilder.HideCategory("MeshSettings");
				DetailBuilder.HideCategory("Positions");
				DetailBuilder.HideCategory("Normals");
				DetailBuilder.HideCategory("Tangents");
				DetailBuilder.HideCategory("UVs");
				DetailBuilder.HideCategory("Turbulence");
				DetailBuilder.HideCategory("SingularMeshSettings");
				DetailBuilder.HideCategory("PerPathSettings");
				DetailBuilder.HideCategory("PlaneMeshSettings");
				DetailBuilder.HideCategory("SplashMeshSettings");
				DetailBuilder.HideCategory("Bulge");
			}

			if (Settings->CurrentTabState == ESH_W2_TabState::W2_TS_Materials)
			{
				IDetailCategoryBuilder& Cat_Materials = DetailBuilder.EditCategory("Materials", FText::GetEmpty(), ECategoryPriority::Default);
			}
			else
			{
				DetailBuilder.HideCategory("Materials");
			}

			if (Settings->CurrentTabState == ESH_W2_TabState::W2_TS_FX)
			{
				TSharedPtr<IPropertyHandle> Prop_FxPathSelected = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, FxPathSelected));
				DetailBuilder.HideProperty(Prop_FxPathSelected);

				IDetailCategoryBuilder& Cat_SFXSettings = DetailBuilder.EditCategory("SFX Settings", FText::GetEmpty(), ECategoryPriority::Default);
				Cat_SFXSettings.AddExternalObjectProperty({ Settings->GetParentWaterfall()->GetSFX_Top() }, GET_MEMBER_NAME_CHECKED(USH_WaterfallSFXComponent, Sound))->DisplayName(FText::FromString("Top SFX"));
				Cat_SFXSettings.AddExternalObjectProperty({ Settings->GetParentWaterfall()->GetSFX_Middle() }, GET_MEMBER_NAME_CHECKED(USH_WaterfallSFXComponent, Sound))->DisplayName(FText::FromString("Middle SFX"));
				Cat_SFXSettings.AddExternalObjectProperty({ Settings->GetParentWaterfall()->GetSFX_Bottom() }, GET_MEMBER_NAME_CHECKED(USH_WaterfallSFXComponent, Sound))->DisplayName(FText::FromString("Bottom SFX"));

				IDetailCategoryBuilder& Cat_TopVfxSettings = DetailBuilder.EditCategory("Top VFX Settings", FText::GetEmpty(), ECategoryPriority::Default);
				CombineTwoPropertyRows(Cat_TopVfxSettings, DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bTopVFX)), DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, TopVfxSystem)));			
				if (Settings->bTopVFX)
				{
					Cat_TopVfxSettings.AddCustomRow(FText::FromString("TopVfxComponent"))
						[
							SNew(SButton)
							.Text(FText::FromString("Focus Component"))
							.HAlign(EHorizontalAlignment::HAlign_Center)
							.OnClicked_Raw(this, &FSH_Details_WaterfallEdit::ButtonClicked_FocusComponent, (USceneComponent*)Settings->GetParentWaterfall()->GetVFX_Top())
						];
				}

				IDetailCategoryBuilder& Cat_MiddleVfxSettings = DetailBuilder.EditCategory("Middle VFX Settings", FText::GetEmpty(), ECategoryPriority::Default);
				{
					CombineTwoPropertyRows(Cat_MiddleVfxSettings, DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bMiddleVFX)), DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, MiddleVfxSystem)));

					if (Settings->bMiddleVFX)
					{
						Cat_MiddleVfxSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bMiddleUsesDistance));

						if (Settings->bMiddleUsesDistance)
						{
							Cat_MiddleVfxSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, DistanceRange));
							Cat_MiddleVfxSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, RemapPoints));
							DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, PointRange));
						}
						else
						{
							Cat_MiddleVfxSettings.AddProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, PointRange));
							DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, DistanceRange));
							DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, RemapPoints));
						}
					}
					else
					{
						DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bMiddleUsesDistance));
						DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, PointRange));
						DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, DistanceRange));
						DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, RemapPoints));
					}

					if (Settings->bMiddleVFX)
					{
						Cat_MiddleVfxSettings.AddCustomRow(FText::FromString("MiddleVfxComponent"))
							[
								SNew(SButton)
								.Text(FText::FromString("Focus Component"))
								.HAlign(EHorizontalAlignment::HAlign_Center)
								.OnClicked_Raw(this, &FSH_Details_WaterfallEdit::ButtonClicked_FocusComponent, (USceneComponent*)Settings->GetParentWaterfall()->GetVFX_Middle())
							];
					}
				}

				IDetailCategoryBuilder& Cat_BottomVfxSettings = DetailBuilder.EditCategory("Bottom VFX Settings", FText::GetEmpty(), ECategoryPriority::Default);
				CombineTwoPropertyRows(Cat_BottomVfxSettings, DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, bBottomVFX)), DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USH_WaterfallSettingsComponent, BottomVfxSystem)));
				if (Settings->bBottomVFX)
				{
					Cat_BottomVfxSettings.AddCustomRow(FText::FromString("BottomVfxComponent"))
						[
							SNew(SButton)
							.Text(FText::FromString("Focus Component"))
							.HAlign(EHorizontalAlignment::HAlign_Center)
							.OnClicked_Raw(this, &FSH_Details_WaterfallEdit::ButtonClicked_FocusComponent, (USceneComponent*)Settings->GetParentWaterfall()->GetVFX_Bottom())
						];
				}

				IDetailCategoryBuilder& Cat_FxIndividualPoints = DetailBuilder.EditCategory("FX (Individual Points)", FText::GetEmpty(), ECategoryPriority::Default);

				Cat_FxIndividualPoints.AddCustomRow(FText::FromString("FX (Individual Points)"))
					.IsEnabled(IsEnabled_FxTabContent())
					[
						SNew(SSH_FxDisplay)
						.Settings(Settings)
						.Prop_PathSelection(Prop_FxPathSelected)
						.RefreshSlate(FSimpleDelegate::CreateRaw(this, &FSH_Details_WaterfallEdit::RefreshSlate))
					];
			}
			else
			{
				DetailBuilder.HideCategory("SFX Settings");
				DetailBuilder.HideCategory("Top VFX Settings");
				DetailBuilder.HideCategory("Middle VFX Settings");
				DetailBuilder.HideCategory("Bottom VFX Settings");
				DetailBuilder.HideCategory("FX (Individual Points)");
			}

			if (Settings->CurrentTabState == ESH_W2_TabState::W2_TS_Bake)
			{
				IDetailCategoryBuilder& Cat_WaterfallSettings = DetailBuilder.EditCategory("WaterfallSettings", FText::GetEmpty(), ECategoryPriority::Default);
				Cat_WaterfallSettings.AddExternalObjectProperty({ Settings->GetParentWaterfall() }, "bShowDynamicMeshesInGame");

				IDetailCategoryBuilder& Cat_BakeSettings = DetailBuilder.EditCategory("BakeSettings", FText::GetEmpty(), ECategoryPriority::Default);

				IDetailCategoryBuilder& Cat_RuntimeMesh = DetailBuilder.EditCategory("RuntimeMesh", FText::GetEmpty(), ECategoryPriority::Default);
				UStaticMeshComponent* BakedMeshComp = Settings->GetParentWaterfall()->GetBakedMeshComp();
				IDetailPropertyRow* Row = Cat_RuntimeMesh.AddExternalObjectProperty({ BakedMeshComp }, "StaticMesh");
				if (Row)
				{
					Row->DisplayName(FText::FromString("Current Runtime Mesh"));
					Row->GetPropertyHandle()->SetOnPropertyValueChanged(FSimpleDelegate::CreateUObject(Settings->GetParentWaterfall(), &ASH_Waterfall2::OnStaticMeshPropChange));
				}
			}
			else
			{
				DetailBuilder.HideCategory("BakeSettings");
			}

			if (Settings->CurrentTabState == ESH_W2_TabState::W2_TS_Debug)
			{
				IDetailCategoryBuilder& Cat_Debug = DetailBuilder.EditCategory("Debug", FText::GetEmpty(), ECategoryPriority::Default);
			}
			else
			{
				DetailBuilder.HideCategory("Debug");
			}
		}
	}
}

bool FSH_Details_WaterfallEdit::IsEnabled_FxTabContent() const
{
	if (Settings)
	{
		if (Settings->GetParentWaterfall())
		{
			return Settings->GetParentWaterfall()->HasGeneratedPaths();
		}
	}

	return false;
}

void FSH_Details_WaterfallEdit::RefreshSlate()
{
	if (PropUtils.IsValid()) PropUtils->ForceRefresh();
};

/*Referenced [FSplinePointDetails] for these functions*/
void FSH_Details_WaterfallEdit::GenerateSplinePointSelectionControls(IDetailCategoryBuilder& ChildrenBuilder)
{
	FMargin ButtonPadding(2.f, 0.f);
	FText RowName = FText::FromString("Select Top Spline Points");

	if (Settings)
	{
		if (Settings->GetParentWaterfall())
		{
			if (Settings->GetParentWaterfall()->GetTopSpline())
			{
				SplineComp_TopSpline = Settings->GetParentWaterfall()->GetTopSpline();

				TSharedPtr<FComponentVisualizer> Visualizer = GUnrealEd->FindComponentVisualizer(USplineComponent::StaticClass());
				SplineVisualizer = StaticCastSharedPtr<FSplineComponentVisualizer>(Visualizer);
			}
		}
	}

	ChildrenBuilder.AddCustomRow(RowName)
	.RowTag("SelectSplinePoints")
	.NameContent()
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(RowName)
	]
	.ValueContent()
	.VAlign(VAlign_Fill)
	.MaxDesiredWidth(170.f)
	.MinDesiredWidth(170.f)
	[
		SNew(SHorizontalBox)
		.Clipping(EWidgetClipping::ClipToBounds)

		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(ButtonPadding)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SplineComponentDetails.SelectFirst")
			.ContentPadding(2.0f)
			.ToolTipText(FText::FromString("Select first spline point."))
			.OnClicked(this, &FSH_Details_WaterfallEdit::OnSelectFirstLastSplinePoint, true)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(ButtonPadding)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SplineComponentDetails.AddPrev")
			.ContentPadding(2.f)
			.ToolTipText(FText::FromString("Add previous spline point to current selection."))
			.OnClicked(this, &FSH_Details_WaterfallEdit::OnSelectPrevNextSplinePoint, false, true)
			.IsEnabled(this, &FSH_Details_WaterfallEdit::ArePointsSelected)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(ButtonPadding)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SplineComponentDetails.SelectPrev")
			.ContentPadding(2.f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.ToolTipText(FText::FromString("Select previous spline point."))
			.OnClicked(this, &FSH_Details_WaterfallEdit::OnSelectPrevNextSplinePoint, false, false)
			.IsEnabled(this, &FSH_Details_WaterfallEdit::ArePointsSelected)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(ButtonPadding)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SplineComponentDetails.SelectAll")
			.ContentPadding(2.f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.ToolTipText(FText::FromString("Select all spline points."))
			.OnClicked(this, &FSH_Details_WaterfallEdit::OnSelectAllSplinePoints)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(ButtonPadding)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SplineComponentDetails.SelectNext")
			.ContentPadding(2.f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.ToolTipText(FText::FromString("Select next spline point."))
			.OnClicked(this, &FSH_Details_WaterfallEdit::OnSelectPrevNextSplinePoint, true, false)
			.IsEnabled(this, &FSH_Details_WaterfallEdit::ArePointsSelected)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(ButtonPadding)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SplineComponentDetails.AddNext")
			.ContentPadding(2.f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.ToolTipText(FText::FromString("Add next spline point to current selection."))
			.OnClicked(this, &FSH_Details_WaterfallEdit::OnSelectPrevNextSplinePoint, true, true)
			.IsEnabled(this, &FSH_Details_WaterfallEdit::ArePointsSelected)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(ButtonPadding)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SplineComponentDetails.SelectLast")
			.ContentPadding(2.f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.ToolTipText(FText::FromString("Select last spline point."))
			.OnClicked(this, &FSH_Details_WaterfallEdit::OnSelectFirstLastSplinePoint, false)
		]
	];
}

FReply FSH_Details_WaterfallEdit::OnSelectFirstLastSplinePoint(bool bFirst)
{
	// Temp fix to move gizmo into right position - doesn't work in 5.4 so it's hidden until a solution can be found
	// Call undo and redo - hacky solution because of the engine bug where it won't set the gizmo correctly when pressing the button for the first time

	if (SplineVisualizer.IsValid())
	{
		bool bIsFirstSelection = SplineVisualizer->GetSelectedKeys().Num() <= 0;

		bool bActivateComponentVis = false;

		if (!SplineComp_TopSpline)
		{
			if (Settings)
			{
				if (Settings->GetParentWaterfall())
				{
					SplineComp_TopSpline = Settings->GetParentWaterfall()->GetTopSpline();
					bActivateComponentVis = true;
				}
			}
		}

		if (SplineComp_TopSpline)
		{
			if (SplineVisualizer->HandleSelectFirstLastSplinePoint(SplineComp_TopSpline, bFirst))
			{
				if (bActivateComponentVis)
				{
					TSharedPtr<FComponentVisualizer> Visualizer = StaticCastSharedPtr<FComponentVisualizer>(SplineVisualizer);
					GUnrealEd->ComponentVisManager.SetActiveComponentVis(GCurrentLevelEditingViewportClient, Visualizer);
				}
			}
		}

		if (bIsFirstSelection)
		{
			GUnrealEd->Exec(GEditor->GetEditorWorldContext(false).World(), TEXT("TRANSACTION UNDO"));
			GUnrealEd->Exec(GEditor->GetEditorWorldContext(false).World(), TEXT("TRANSACTION REDO"));
		}
	}

	return FReply::Handled();
}

FReply FSH_Details_WaterfallEdit::OnSelectPrevNextSplinePoint(bool bNext, bool bAddToSelection)
{
	if (SplineVisualizer.IsValid())
	{
		bool bIsFirstSelection = SplineVisualizer->GetSelectedKeys().Num() <= 0;

		SplineVisualizer->OnSelectPrevNextSplinePoint(bNext, bAddToSelection);

		if (bIsFirstSelection)
		{
			GUnrealEd->Exec(GEditor->GetEditorWorldContext(false).World(), TEXT("TRANSACTION UNDO"));
			GUnrealEd->Exec(GEditor->GetEditorWorldContext(false).World(), TEXT("TRANSACTION REDO"));
		}
	}
	return FReply::Handled();
}

FReply FSH_Details_WaterfallEdit::OnSelectAllSplinePoints()
{
	if (SplineVisualizer.IsValid())
	{
		bool bIsFirstSelection = SplineVisualizer->GetSelectedKeys().Num() <= 0;

		bool bActivateComponentVis = false;

		if (!SplineComp_TopSpline)
		{
			if (Settings)
			{
				if (Settings->GetParentWaterfall())
				{
					SplineComp_TopSpline = Settings->GetParentWaterfall()->GetTopSpline();
					bActivateComponentVis = true;
				}
			}
		}

		if (SplineComp_TopSpline)
		{
			if (SplineVisualizer->HandleSelectAllSplinePoints(SplineComp_TopSpline))
			{
				if (bActivateComponentVis)
				{
					TSharedPtr<FComponentVisualizer> Visualizer = StaticCastSharedPtr<FComponentVisualizer>(SplineVisualizer);
					GUnrealEd->ComponentVisManager.SetActiveComponentVis(GCurrentLevelEditingViewportClient, Visualizer);
				}
			}
		}

		if (bIsFirstSelection)
		{
			GUnrealEd->Exec(GEditor->GetEditorWorldContext(false).World(), TEXT("TRANSACTION UNDO"));
			GUnrealEd->Exec(GEditor->GetEditorWorldContext(false).World(), TEXT("TRANSACTION REDO"));
		}
	}
	return FReply::Handled();
}

bool FSH_Details_WaterfallEdit::ArePointsSelected() const
{
	return SplineVisualizer.IsValid() ? SplineVisualizer->GetSelectedKeys().Num() > 0 : false;
}

FReply FSH_Details_WaterfallEdit::ButtonClicked_FocusComponent(USceneComponent* Component)
{
	if (GEditor)
	{
		USelection* SelectedComps = GEditor->GetSelectedComponents();
		if (SelectedComps) SelectedComps->DeselectAll();
		GEditor->SelectComponent(Component, true, true, true);
		GEditor->NoteSelectionChange();
	}

	return FReply::Handled();
}