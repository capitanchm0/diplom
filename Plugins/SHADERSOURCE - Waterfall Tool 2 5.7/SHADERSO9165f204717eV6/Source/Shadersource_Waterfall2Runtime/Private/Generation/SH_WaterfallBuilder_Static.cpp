// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Generation/SH_WaterfallBuilder_Static.h"

#if WITH_EDITOR
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "EditorComponents/SH_WaterfallMeshComponent.h"
#include "SH_WaterfallTool2Statics.h"
#include "Actors/SH_Waterfall2.h"

#include "EditorAssetLibrary.h"
#include "FileHelpers.h"
#include "StaticMeshAttributes.h"

#include "GeometryScript/MeshMaterialFunctions.h"
#include "GeometryScript/MeshSelectionFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "GeometryScript/CreateNewAssetUtilityFunctions.h"

void FSH_WaterfallBuilder_Static::BakeToStaticMesh(FSimpleDelegate OnProcessingFinished)
{
	TArray<USH_WaterfallMeshComponent*> MeshComponents = GetParentWaterfall()->GetAllMeshComponents();

	//Create a temporary component to combine all meshes into one
	if (USH_WaterfallMeshComponent* CombinedMeshComp = NewObject<USH_WaterfallMeshComponent>(GetWaterfallSettings()->GetOuter()))
	{
		CombinedMeshComp->SetupAttachment(GetParentWaterfall()->GetRootComponent());
		GetParentWaterfall()->AddInstanceComponent(CombinedMeshComp);
		GetParentWaterfall()->AddOwnedComponent(CombinedMeshComp);
		CombinedMeshComp->RegisterComponent();

		UDynamicMesh* CombinedMesh = CombinedMeshComp->GetDynamicMesh();
		UGeometryScriptLibrary_MeshMaterialFunctions::EnableMaterialIDs(CombinedMesh);

		//Get all of the materials
		TArray<FStaticMaterial> StaticMaterials = {};
		for (USH_WaterfallMeshComponent* CurrentMeshComp : MeshComponents)
		{
			//We only want to work with meshes that have actually been generated
			if (CurrentMeshComp->GetDynamicMesh()->GetTriangleCount() > 0)
			{
				int32 MaterialArrayIndex = (GetWaterfallSettings()->bCombineSameMaterials) ? INDEX_NONE : StaticMaterials.Num();
				if (GetWaterfallSettings()->bCombineSameMaterials)
				{
					for (int i = 0; i < StaticMaterials.Num(); i++)
					{
						if (StaticMaterials[i].MaterialInterface == CurrentMeshComp->GetMaterial(0))
						{
							MaterialArrayIndex = i;
							break;
						}
					}
				}

				if (!GetWaterfallSettings()->bCombineSameMaterials || (GetWaterfallSettings()->bCombineSameMaterials && MaterialArrayIndex == INDEX_NONE))
				{
					StaticMaterials.Add(FStaticMaterial(CurrentMeshComp->GetMaterial(0), FName(StringHelpers::ToString(CurrentMeshComp->MeshType))));
					MaterialArrayIndex = StaticMaterials.Num() - 1;
				}
				else //It already exists as a slot
				{
					FName CurrentName = StaticMaterials[StaticMaterials.Num() - 1].MaterialSlotName;
					StaticMaterials[StaticMaterials.Num() - 1].MaterialSlotName = FName(CurrentName.ToString() + "_" + StringHelpers::ToString(CurrentMeshComp->MeshType));
				}

				UDynamicMesh* CurrentMesh = CurrentMeshComp->GetDynamicMesh();

				//Select all triangles of the current combined mesh
				FGeometryScriptMeshSelection SelectionB;
				UGeometryScriptLibrary_MeshSelectionFunctions::CreateSelectAllMeshSelection(CombinedMesh, SelectionB);

				//Add the current mesh to the combined mesh
				UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(CombinedMesh, CurrentMesh, FTransform::Identity);

				//Select all trianges of the new combined mesh
				FGeometryScriptMeshSelection SelectionA;
				UGeometryScriptLibrary_MeshSelectionFunctions::CreateSelectAllMeshSelection(CombinedMesh, SelectionA);

				//Subtract the original triangles before we combined the mesh from the new combined mesh to get a selection of JUST the triangles we just added in the append
				FGeometryScriptMeshSelection SelectionC;
				UGeometryScriptLibrary_MeshSelectionFunctions::CombineMeshSelections(SelectionA, SelectionB, SelectionC, EGeometryScriptCombineSelectionMode::Subtract);

				//Assign the new triangles the correct Material Slot ID
				UGeometryScriptLibrary_MeshMaterialFunctions::SetMaterialIDForMeshSelection(CombinedMesh, SelectionC, MaterialArrayIndex);
			}
		}

		FString NewSaveObjectPath = USH_WaterfallTool2Statics::SummonFilePicker_ContentBrowser("Save Waterfall Static Mesh", "NewWaterfallMesh", GetWaterfallSettings()->LastBakePath, { UStaticMesh::StaticClass()->GetClassPathName() });
		if (!NewSaveObjectPath.IsEmpty()) //If it's empty it means the user cancelled
		{
			GetWaterfallSettings()->LastBakePath = FPaths::GetPath(NewSaveObjectPath);

			//Create the static mesh asset and assign it to the runtime waterfall
			EGeometryScriptOutcomePins Outcome;
			FGeometryScriptCreateNewStaticMeshAssetOptions Options = FGeometryScriptCreateNewStaticMeshAssetOptions();
			Options.bEnableCollision = false;

			//If any asset already exists, we have to delete it first - the create mesh fucntion doesn't allow us to take in a previous package,
			//so we have to create a new one with the same name - therefore we have to delete the previous package so there's no confusion about loaded packages
			FAssetData ExistingAsset = USH_WaterfallTool2Statics::GetAssetByPath(NewSaveObjectPath);
			if (ExistingAsset.IsValid())
			{
				UEditorAssetLibrary::DeleteAsset(ExistingAsset.GetObjectPathString());
			}

			UStaticMesh* SavedMesh = UGeometryScriptLibrary_CreateNewAssetFunctions::CreateNewStaticMeshAssetFromMesh(CombinedMesh, FPackageName::ObjectPathToPackageName(NewSaveObjectPath), Options, Outcome);
			if (SavedMesh)
			{
				/* Re-populate section info map - referenced from [UE::AssetUtils::CreateStaticMeshAsset]
				* The addition of this code into the CreateStaticMeshAsset in 5.4 caused errors when creating the mesh
				* because it was trying to reference material slot names that hadn't been set yet.
				* Therefore this part of the code is done again here to override the engine code and set eveything correctly. */
				{
					FMeshSectionInfoMap SectionInfoMap;
					for (int32 LODIndex = 0; LODIndex < 1; ++LODIndex) //If LODs are ever added, this will change from 1 to num LODs
					{
						if (FMeshDescription* Mesh = SavedMesh->GetMeshDescription(LODIndex))
						{
							int32 SectionIndex = 0;
							for (FPolygonGroupID PolygonGroupID : Mesh->PolygonGroups().GetElementIDs())
							{
								SectionInfoMap.Set(LODIndex, SectionIndex, FMeshSectionInfo(SectionIndex));
								SectionIndex++;
							}
						}
					}
					SavedMesh->GetSectionInfoMap().CopyFrom(SectionInfoMap);
					SavedMesh->GetOriginalSectionInfoMap().CopyFrom(SectionInfoMap);

					SavedMesh->PostEditChange();
				}

				//Can't just straight up set them otherwise it removes UV data for the material slots
				TArray<FStaticMaterial>& SM = SavedMesh->GetStaticMaterials();
				for (int i = 0; i < SM.Num() && i < StaticMaterials.Num(); i++)
				{
					SM[i].MaterialInterface = StaticMaterials[i].MaterialInterface;
					SM[i].MaterialSlotName = StaticMaterials[i].MaterialSlotName;
				}

				FEditorFileUtils::PromptForCheckoutAndSave({ SavedMesh->GetPackage() }, false, /*bPromptToSave=*/ false);

				if (GetParentWaterfall())
				{
					GetParentWaterfall()->SetStaticMesh(SavedMesh);
					if (GetParentWaterfall()->GetBakedMeshComp())
					{
						GetParentWaterfall()->GetBakedMeshComp()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					}
				}

				USH_WaterfallTool2Statics::PrintEditorNotification("Mesh baking succeeded!");
			}
			else
			{
				USH_WaterfallTool2Statics::PrintEditorNotification("Mesh baking failed!", ESH_W2_NotificationState::W2_NS_Fail);
			}
		}
		else
		{
			USH_WaterfallTool2Statics::PrintEditorNotification("Mesh baking cancelled!", ESH_W2_NotificationState::W2_NS_Fail);
		}

		//Remove the temp component
		GetParentWaterfall()->RemoveInstanceComponent(CombinedMeshComp);
		GetParentWaterfall()->RemoveOwnedComponent(CombinedMeshComp);
		CombinedMeshComp->DestroyComponent();
	}

	OnProcessingFinished.ExecuteIfBound();
}
#endif