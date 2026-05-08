// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Generation/SH_WaterfallBuilder_Path.h"

#if WITH_EDITOR
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "EditorComponents/SH_WaterfallPathComponent.h"
#include "Components/SH_WaterfallSFXComponent.h"
#include "SH_WaterfallTool2Statics.h"
#include "Actors/SH_Waterfall2.h"

#include "Kismet/KismetMathLibrary.h"

void FSH_WaterfallBuilder_Path::StartSimulate(FSimpleDelegate OnProcessingFinished)
{
	if (!IsProcessing())
	{
		if (GetWaterfallSettings())
		{
			OnEndProcessing = OnProcessingFinished;

			const FScopedTransaction Transaction(FText::FromString("Generate Paths"));

			//Remove any existing paths
			DeleteAllPaths();

			//Create paths
			for (int i = 0; i < GetWaterfallSettings()->GetNumPaths(); i++)
			{
				CreatePath(i);
			}

			if (PathsToProcess.Num() > 0)
			{
				StartProcessing();

				PathsProcessing = PathsToProcess;
			}
			else
			{
				USH_WaterfallTool2Statics::PrintEditorNotification("Failed to create any paths!");
				EndSimulate();
			}
		}
		else
		{
			USH_WaterfallTool2Statics::PrintEditorNotification("[FSH_WaterfallBuilder_Path::StartSimulate]: Unable to get Settings Component!");
			EndSimulate();
		}
	}
	else
	{
		EndSimulate();
	}
}

void FSH_WaterfallBuilder_Path::CancelSimulate()
{
	ProcessingFinished();

	DeleteAllPaths();

	USH_WaterfallTool2Statics::PrintEditorNotification("Path generation cancelled!", ESH_W2_NotificationState::W2_NS_Fail);
}

void FSH_WaterfallBuilder_Path::SanitiseProcessedPaths()
{
	//Verifies that all the paths are sill valid because some may have been deleted by a user
	for (int i = ProcessedPaths.Num() - 1; i >= 0; i--)
	{
		if (!ProcessedPaths[i])
		{
			ProcessedPaths.RemoveAt(i);
		}
	}
}

void FSH_WaterfallBuilder_Path::DeleteAllPaths(bool bClearDynamicMeshesToo)
{
	//Clean up any  USH_WaterfallPathComponents
	TArray<USH_WaterfallPathComponent*> WaterfallPaths = GetParentWaterfall() ? GetParentWaterfall()->GetAllPathComponents() : TArray<USH_WaterfallPathComponent*>();

	GEngine->ForceGarbageCollection(true); //So that the names can be released before any other paths are created, in case GC is slow

	for (int i = WaterfallPaths.Num() - 1; i >= 0; i--)
	{
		FString NewName = "DestroyedPath_";
		FName UniqueName = MakeUniqueObjectName(WaterfallPaths[i]->GetOuter(), USH_WaterfallPathComponent::StaticClass(), FName(NewName));
		WaterfallPaths[i]->Rename(*UniqueName.ToString(), WaterfallPaths[i]->GetOuter());

		WaterfallPaths[i]->DestroyComponent();
	}

	ProcessedPaths.Empty();
	PathsProcessing.Empty();
	PathsToProcess.Empty();

	//Also remove the meshes - they will need to be regenerated if we're regenerating the paths
	if (bClearDynamicMeshesToo) GetParentWaterfall()->ClearDynamicMeshes();

	//If the paths are being deleted, then the FX components should also be deleted since they will no longer line up correctly
	GetParentWaterfall()->DeleteAllComponents_FX(false, true);
	GetWaterfallSettings()->FxPathSelected = INDEX_NONE;
}

void FSH_WaterfallBuilder_Path::Tick(float DeltaTime)
{
	//If Modes are changed while a simulation is running, it could cause an editor crash so we need to check if the ptr is still valid
	if (!GetWaterfallSettings())
	{
		USH_WaterfallTool2Statics::PrintEditorNotification("Waterfall Settings Invalid!\nTerminating simulation.", ESH_W2_NotificationState::W2_NS_Fail);
		EndSimulate();
	}

	if (GetWaterfallSettings()->GetSimulationDeltaTime() > 0.f)
	{
		//Split the work up into substeps so that it spreads out over multiple ticks and doesn't kill the engine's fps
		int32 CurrentFrameIterations = 0;
		while (CurrentFrameIterations < GetWaterfallSettings()->GetSubsteps()
			&& PathsToProcess.Num() > 0)
		{
			TArray<USH_WaterfallPathComponent*> DeviationPaths = ProcessedPaths;
			DeviationPaths.Append(PathsProcessing);

			if (PathsToProcess[0]->SimulatePath(GetWaterfallSettings()->GetSimulationDeltaTime(), CurrentFrameIterations, DeviationPaths))
			{
				//Resample path if option selected
				if (GetWaterfallSettings()->bResampleSpline)
				{
					PathsToProcess[0]->ResamplePath(GetWaterfallSettings()->NumSplinePoints);
				}

				NumPathsProcessed++;
				PathsToProcess.RemoveAt(0);

				if (PathsToProcess.Num() <= 0)
				{
					USH_WaterfallTool2Statics::PrintMessageToLog("Path simulation finished for " + GetParentWaterfall()->GetActorLabel());
					EndSimulate();
					break; //We need to break after EndSimulate() because we clear the GetWaterfallSettings() ptr in that function, and we reference it in the while check
				}
			}
		}
	}
	else EndSimulate();
}

void FSH_WaterfallBuilder_Path::EndSimulate()
{
	bool bSuccess = true;

	//Verify that all the paths have been successfully simulated
	for (USH_WaterfallPathComponent* PathComp : PathsProcessing)
	{
		if (PathComp)
		{
			if (!PathComp->HasBeenSimulated())
			{
				bSuccess = false;
				break;
			}
		}
		else
		{
			bSuccess = false;
			break;
		}
	}
	if (bSuccess) //Register the paths
	{
		for (USH_WaterfallPathComponent* PathComp : PathsProcessing)
		{
			RegisterPath(PathComp);
		}

		GEngine->ForceGarbageCollection(true); //So that the names can be released before any other paths are created, in case GC is slow

		for (int i = 0; i < ProcessedPaths.Num(); i++)
		{
			FString NewName = "GeneratedPath" + FString::SanitizeFloat(i, 0);
			UObject* ExistingObject = StaticFindObject(/*Class=*/ nullptr, ProcessedPaths[i]->GetOuter(), *NewName, EFindObjectFlags::ExactClass);
			if (!ExistingObject) //This is just a precaution to avoid crashes
			{
				ProcessedPaths[i]->Rename(*NewName, ProcessedPaths[i]->GetOuter());
			}
			ProcessedPaths[i]->PathIndex = i;
		}

		//Set the FxPathSelected variable to half so that it's in the middle
		GetWaterfallSettings()->FxPathSelected = ProcessedPaths.Num() / 2;

		//Set the SFX Components to the top, mid, and bottom locations of the middle path by default
		USH_WaterfallPathComponent* MidPath = ProcessedPaths[GetWaterfallSettings()->FxPathSelected];
		if (MidPath)
		{
			GetParentWaterfall()->GetSFX_Top()->SetWorldLocation(MidPath->GetLocationAtTime(0.f, ESplineCoordinateSpace::World));
			GetParentWaterfall()->GetSFX_Middle()->SetWorldLocation(MidPath->GetLocationAtTime(0.5f, ESplineCoordinateSpace::World));
			GetParentWaterfall()->GetSFX_Bottom()->SetWorldLocation(MidPath->GetLocationAtTime(1.f, ESplineCoordinateSpace::World));
		}

		USH_WaterfallTool2Statics::PrintEditorNotification("Path generation successful!");

		GetParentWaterfall()->MarkPackageDirty();
	}
	else //Destroy all the paths because the simulation failed
	{
		DeleteAllPaths();
		USH_WaterfallTool2Statics::PrintEditorNotification("Path generation failed!", ESH_W2_NotificationState::W2_NS_Fail);
	}

	GetParentWaterfall()->RefreshDetailsPanel();
	ProcessingFinished();
}

void FSH_WaterfallBuilder_Path::CreatePath(int32 Index)
{
	float CalculatedNormalisedSplineDistance = (GetWaterfallSettings()->GetNumPaths() > 1)
		? FMath::GetMappedRangeValueClamped(TRange<float>(0.f, 1.f), TRange<float>(GetWaterfallSettings()->GetPathSpawnRange().X, GetWaterfallSettings()->GetPathSpawnRange().Y), (float)Index / FMath::Max<float>(GetWaterfallSettings()->GetNumPaths() - 1, 1))
		: GetWaterfallSettings()->GetSinglePathPosition();

	//Add to Paths to Process if successful, print error to log if unsuccessful
	if (USH_WaterfallPathComponent* NewPath = NewObject<USH_WaterfallPathComponent>(GetWaterfallSettings()->GetOuter()))
	{
		NewPath->SetupAttachment(GetParentWaterfall()->GetTopSpline());
		//NewPath->MarkAsEditorOnlySubobject(); //This component is Editor-Only and should not be included in packaged builds - this makes it so that it doesn't show up in the Details Panel
		GetParentWaterfall()->AddInstanceComponent(NewPath);
		GetParentWaterfall()->AddOwnedComponent(NewPath);
		NewPath->RegisterComponent();

		//The new component is attached to the spline, so we need to work in local space
		FTransform LocalComponentTransform = FTransform(
			UKismetMathLibrary::MakeRotationFromAxes(
				GetParentWaterfall()->GetTopSpline()->GetRightVectorAtTime(CalculatedNormalisedSplineDistance, ESplineCoordinateSpace::Local, true),
				GetParentWaterfall()->GetTopSpline()->GetDirectionAtTime(CalculatedNormalisedSplineDistance, ESplineCoordinateSpace::Local, true),
				GetParentWaterfall()->GetTopSpline()->GetUpVectorAtTime(CalculatedNormalisedSplineDistance, ESplineCoordinateSpace::Local, true)
			),
			GetParentWaterfall()->GetTopSpline()->GetLocationAtTime(CalculatedNormalisedSplineDistance, ESplineCoordinateSpace::Local, true),
			FVector::OneVector
		);

		NewPath->SetRelativeTransform(LocalComponentTransform);

		if (GetWaterfallSettings()->GetAddSpawnJitter()) //Simple mode doesn't allow for spawn jitter
		{
			/*Create a new "predicatable" seed - if we use the same seed for every path, the same random
			number is going to be returned because we're operating the randoms on the same frame.
			Therefore, this uses a magic number algorithm to modify the seed based on the Index so that it
			can get calculated the same every time for the same seed and give a different number each time.*/
			int32 Seed = GetWaterfallSettings()->Random().GetInitialSeed();
			if (Seed >= 10000)
			{
				if (Index % 2 == 0) Seed /= (FMath::Square(Index + 1) / 3 + 7);
				else Seed /= (FMath::Square(Index + 1) / 2 + 5);
			}
			else
			{
				if (Index % 2 == 0) Seed *= (FMath::Square(Index + 1) / 3 + 7);
				else Seed *= (FMath::Square(Index + 1) / 2 + 5);
			}
			FRandomStream JitterSeededRandom = FRandomStream(Seed);
			float RandomYaw = JitterSeededRandom.FRandRange(GetWaterfallSettings()->GetSpawnJitterMin(), GetWaterfallSettings()->GetSpawnJitterMax());

			NewPath->SetRelativeRotation(NewPath->GetRelativeRotation() + FRotator(0.f, RandomYaw, 0.f));
		}

		NewPath->NormalisedSplineDistance = CalculatedNormalisedSplineDistance;
		NewPath->SplineDistance = GetParentWaterfall()->GetTopSpline()->GetSplineLength() * CalculatedNormalisedSplineDistance;

		//Seed used for UV's only
		NewPath->UVSeed = FMath::FRand();

		NewPath->CalculateIntialSimulationVariables();

		PathsToProcess.Add(NewPath);
	}
	else
	{
		USH_WaterfallTool2Statics::PrintMessageToLog("[FSH_WaterfallBuilder_Path::CreatePath()] Failed to create new USH_WaterfallPathComponent.", ELogVerbosity::Warning);
	}
}

bool FSH_WaterfallBuilder_Path::RegisterPath(USH_WaterfallPathComponent* PathToRegister)
{
	if (PathToRegister)
	{
		//Orders the paths correctly
		SanitiseProcessedPaths();

		if (ProcessedPaths.Num() <= 0)
		{
			ProcessedPaths.Add(PathToRegister);
			return true;
		}
		else
		{
			//Compare NormalisedSplineDistance so we know where to add it in the array
			for (int i = 0; i < ProcessedPaths.Num(); i++)
			{
				if (FMath::Clamp(ProcessedPaths[i]->NormalisedSplineDistance, 0.f, 1.f) >= FMath::Clamp(PathToRegister->NormalisedSplineDistance, 0.f, 1.f))
				{
					ProcessedPaths.Insert(PathToRegister, i);
					return true;
				}
			}

			//If we reach the end of the loop then the path just needs to go at the end of the array
			ProcessedPaths.Add(PathToRegister);
			return true;
		}
	}

	return false;
}

void FSH_WaterfallBuilder_Path::ProcessingFinished()
{
	FSH_WaterfallBuilder::ProcessingFinished();
	


	OnEndProcessing.ExecuteIfBound();
}
#endif