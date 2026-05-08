// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Generation/SH_WaterfallBuilder_Mesh.h"

#if WITH_EDITOR
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "EditorComponents/SH_WaterfallPathComponent.h"
#include "EditorComponents/SH_WaterfallMeshComponent.h"
#include "SH_WaterfallTool2Statics.h"
#include "Actors/SH_Waterfall2.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include "GeometryScript/MeshUVFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "GeometryScript/MeshTransformFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/MeshPolygroupFunctions.h"
#include "GeometryScript/MeshSelectionFunctions.h"

void FSH_WaterfallBuilder_Mesh::StartMeshGeneration(FSimpleDelegate OnProcessingFinished)
{
	if (!IsProcessing())
	{
		if (GetWaterfallSettings())
		{
			if (QueuedMeshesToBeGenerated.Num() > 0)
			{
				GetWaterfallSettings()->SelectedMesh = QueuedMeshesToBeGenerated[0];
				QueuedMeshesToBeGenerated.RemoveAt(0);

				OnEndProcessing = OnProcessingFinished;

				const FScopedTransaction Transaction(FText::FromString("Generate Mesh"));

				//Clear any meshes already generated
				GetParentWaterfall()->ClearDynamicMeshes(GetWaterfallSettings()->SelectedMesh);

				TArray<USH_WaterfallPathComponent*> WaterfallPaths = GetParentWaterfall()->GetAllPathComponents();
				if (WaterfallPaths.Num() > 0)
				{
					//Get the paths to process
					if (GetWaterfallSettings()->bGenerateMeshForPathsInRange)
					{
						PathsToProcess.Empty();
						PathsProcessing.Empty();
						for (int i = GetWaterfallSettings()->GetPathRangeMinChecked(); i <= GetWaterfallSettings()->GetPathRangeMaxChecked(WaterfallPaths.Num() - 1); i++)
						{
							PathsToProcess.Add(WaterfallPaths[i]);
						}

						PathsProcessing = PathsToProcess;
					}
					else
					{
						PathsToProcess = WaterfallPaths;
						PathsProcessing = WaterfallPaths;
					}
				}
				else
				{
					USH_WaterfallTool2Statics::PrintEditorNotification("No paths to process!", ESH_W2_NotificationState::W2_NS_Fail);
					CancelMeshGeneration(false);
					return;
				}

				for (USH_WaterfallPathComponent* WaterfallPath : PathsProcessing)
				{
					//We also need to invalidate the caches in case a path has been manually modified
					WaterfallPath->InvalidateSampleCache();
					WaterfallPath->InvalidatePointsCache();
				}

				//2 paths minimum for generating a singular mesh, otherwise 1 path minimum for any other type
				if ((GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_Singular && PathsProcessing.Num() > 1)
					|| (GetWaterfallSettings()->SelectedMesh != ESH_MeshGenerationType::W2_MG_Singular && PathsProcessing.Num() > 0))
				{
					StartProcessing();

					ApplySamplingSettings();
					ClearMeshBuffers();
				}
				else
				{
					USH_WaterfallTool2Statics::PrintEditorNotification("Minimum paths for generation not met for mesh generation type!", ESH_W2_NotificationState::W2_NS_Fail);
					CancelMeshGeneration(false);
				}
			}
			else
			{
				USH_WaterfallTool2Statics::PrintEditorNotification("No meshes queued!", ESH_W2_NotificationState::W2_NS_Fail);
				CancelMeshGeneration(false);
			}
		}
		else
		{
			USH_WaterfallTool2Statics::PrintEditorNotification("[FSH_WaterfallBuilder_Mesh::StartMeshGeneration]: Failed to get Settings Component!", ESH_W2_NotificationState::W2_NS_Fail);
			CancelMeshGeneration(false);
		}
	}
	else
	{
		CancelMeshGeneration(false);
	}
}

void FSH_WaterfallBuilder_Mesh::CancelMeshGeneration(bool bShowEditorNotification)
{
	QueuedMeshesToBeGenerated.Empty();
	ProcessingFinished();
	GetParentWaterfall()->ClearDynamicMeshes(GetWaterfallSettings()->IsSimple() ? ESH_MeshGenerationType::W2_MG_All : GetWaterfallSettings()->SelectedMesh);

	if (bShowEditorNotification) USH_WaterfallTool2Statics::PrintEditorNotification("Mesh generation cancelled!", ESH_W2_NotificationState::W2_NS_Fail);
}

void FSH_WaterfallBuilder_Mesh::Tick(float DeltaTime)
{
	if (GetWaterfallSettings())
	{
		if (GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_Singular)
		{
			if (GetWaterfallSettings()->bSingularMeshSubdivide) TickMeshGeneration_SingularRemesh(DeltaTime);
			else TickMeshGeneration_SingularNoRemesh(DeltaTime);
		}
		else TickMeshGeneration_OnePath(DeltaTime);
	}
}

void FSH_WaterfallBuilder_Mesh::TickMeshGeneration_SingularRemesh(float DeltaTime)
{
	//While we have subdivisions left to iterate on
	if (NumPathsProcessed < GetWaterfallSettings()->SingularMeshSubdivisions.X)
	{
		if (!bSingularCachedPaths)
		{
			for (USH_WaterfallPathComponent* WaterfallPath : PathsToProcess)
			{
				if (!WaterfallPath->CachePath()) return; //Wait until all the paths are cached before continuing
			}

			//If we reach the end of the for loop without returning, all the paths are cached
			bSingularCachedPaths = true;

			//Remap the points
			SingularRemappedPaths = GetWaterfallSettings()->bSortPoints ? Singular_RemapPoints(false) : Singular_CovertToRemappedPaths();
		}

		//Work out the subdivisions
		float PathAlpha = (float)NumPathsProcessed / (float)(GetWaterfallSettings()->SingularMeshSubdivisions.X - 1);
		float IndexedAlpha = PathAlpha * (SingularRemappedPaths.Num() - 1);
		float PathInterpAlpha = FMath::Fractional(IndexedAlpha);
		int32 PathIndex = FMath::FloorToInt(IndexedAlpha);
		int32 NextPathIndex = FMath::Min(PathIndex + 1, SingularRemappedPaths.Num() - 1);

		if (PathIndex < 0 || PathIndex >= SingularRemappedPaths.Num()
			|| NextPathIndex < 0 || NextPathIndex >= SingularRemappedPaths.Num())
		{
			EndMeshGeneration();
			return;
		}

		//Build the triangles and vertices arrays to feed into the dynamic mesh in EndMeshGeneration()
		BuildMeshBuffers_SingularRemesh(SingularRemappedPaths[PathIndex], SingularRemappedPaths[NextPathIndex], PathInterpAlpha, PathAlpha, PathIndex);

		//Check if we are now finished
		NumPathsProcessed++;
		if (NumPathsProcessed > GetWaterfallSettings()->SingularMeshSubdivisions.X) EndMeshGeneration();
	}
	else
	{
		EndMeshGeneration();
	}
}

void FSH_WaterfallBuilder_Mesh::TickMeshGeneration_SingularNoRemesh(float DeltaTime)
{
	if (!bSingularCachedPaths)
	{
		for (USH_WaterfallPathComponent* WaterfallPath : PathsToProcess)
		{
			if (!WaterfallPath->CachePath()) return; //Wait until all the paths are cached before continuing
		}

		//If we reach the end of the for loop without returning, all the paths are cached
		bSingularCachedPaths = true;

		//Remap the points
		SingularRemappedPaths = GetWaterfallSettings()->bSortPoints ? Singular_RemapPoints(false) : Singular_CovertToRemappedPaths();
	}

	//Get the next two paths to generate mesh data for if there are still two left
	if (SingularRemappedPaths.Num() >= 2)
	{
		//Build the triangles and vertices arrays to feed into the dynamic mesh in EndMeshGeneration()
		BuildMeshBuffers_SingularNoRemesh(SingularRemappedPaths[0], SingularRemappedPaths[1]);

		NumPathsProcessed++;

		//Remove the path already mapped and finish now if there's only one path left
		SingularRemappedPaths.RemoveAt(0);
		if (SingularRemappedPaths.Num() < 2) EndMeshGeneration();
	}
	else
	{
		EndMeshGeneration();
	}
}

void FSH_WaterfallBuilder_Mesh::TickMeshGeneration_OnePath(float DeltaTime)
{
	//Get the next path to generate mesh data for if there are any left
	if (PathsToProcess.Num() >= 1)
	{
		USH_WaterfallPathComponent* CurrentPath = PathsToProcess[0];

		if (!CurrentPath)
		{
			EndMeshGeneration();
		}
		else
		{
			//Cache the path if it's not already done so
			if (!CurrentPath->CachePath()) return; //Wait

			//Build the triangles and vertices arrays to feed into the dynamic mesh in EndMeshGeneration()
			switch (GetWaterfallSettings()->SelectedMesh)
			{
			case ESH_MeshGenerationType::W2_MG_PerPath: BuildMeshBuffers_OnePath(CurrentPath, GetWaterfallSettings()->PerPathSubdivisions); break;
			case ESH_MeshGenerationType::W2_MG_Cross:	BuildMeshBuffers_OnePath(CurrentPath, GetWaterfallSettings()->CrossPlaneSubdivisions); break;
			case ESH_MeshGenerationType::W2_MG_Splash:	BuildMeshBuffers_Splash(CurrentPath); break;
			}

			NumPathsProcessed++;

			//Remove the path already mapped and finish now if there's only one path left
			PathsToProcess.RemoveAt(0);
			if (PathsToProcess.Num() <= 0)
			{
				EndMeshGeneration();
			}
		}
	}
	else
	{
		EndMeshGeneration();
	}
}

void FSH_WaterfallBuilder_Mesh::EndMeshGeneration()
{
	if (/*ProcessingType == ESH_W2_ProcessingType::W2_PT_Mesh &&*/ IsProcessing())
	{
		if (Positions.Num() > 0 && Triangles.Num() > 0)
		{
			//GeneratedMeshes
			USH_WaterfallMeshComponent* DynamicMeshComp = GetParentWaterfall()->GetMeshForType(GetWaterfallSettings()->SelectedMesh);
			if (DynamicMeshComp)
			{
				//Apply the mesh buffers
				int32 TriCount = 0;
				{
					DynamicMeshComp->SetMaterial(0, GetWaterfallSettings()->GetMaterialForMesh());
					UDynamicMesh* DynamicMesh = DynamicMeshComp->GetDynamicMesh();
					if (DynamicMesh)
					{
						//Create the geo
						UGeometryScriptLibrary_MeshUVFunctions::SetNumUVSets(DynamicMesh, 4);

						FGeometryScriptSimpleMeshBuffers MeshBuffers = FGeometryScriptSimpleMeshBuffers();
						MeshBuffers.Vertices = Positions;
						MeshBuffers.Normals = Normals;
						MeshBuffers.UV0 = UVs0;
						MeshBuffers.UV1 = UVs1;
						MeshBuffers.UV2 = UVs2;
						MeshBuffers.UV3 = UVs3;
						MeshBuffers.VertexColors = Colours;
						MeshBuffers.Triangles = Triangles;

						FGeometryScriptIndexList OutList;
						UGeometryScriptLibrary_MeshBasicEditFunctions::AppendBuffersToMesh(DynamicMesh, MeshBuffers, OutList);

						//Make the mesh relative to the actor
						UGeometryScriptLibrary_MeshTransformFunctions::TransformMesh(DynamicMesh, GetParentWaterfall()->GetActorTransform().Inverse());

						//Recompute normals if auto compute is enabled
						if (GetWaterfallSettings()->bAutoComputeNormals)
						{
							FGeometryScriptCalculateNormalsOptions NormalsOptions = FGeometryScriptCalculateNormalsOptions();
							NormalsOptions.bAngleWeighted = true;
							NormalsOptions.bAreaWeighted = true;
							UGeometryScriptLibrary_MeshNormalsFunctions::RecomputeNormals(DynamicMesh, NormalsOptions);
						}

						//Compute the tangents
						FGeometryScriptTangentsOptions TangentsOptions = FGeometryScriptTangentsOptions();
						TangentsOptions.Type = EGeometryScriptTangentTypes::FastMikkT;
						TangentsOptions.UVLayer = 1;
						UGeometryScriptLibrary_MeshNormalsFunctions::ComputeTangents(DynamicMesh, TangentsOptions);

						TriCount = DynamicMesh->GetTriangleCount();

						//Assign the polygroup on the seconsary layer for the ID
						UGeometryScriptLibrary_MeshPolygroupFunctions::EnablePolygroups(DynamicMesh);
						UGeometryScriptLibrary_MeshPolygroupFunctions::SetNumExtendedPolygroupLayers(DynamicMesh, 1);
						FGeometryScriptMeshSelection Selection;
						UGeometryScriptLibrary_MeshSelectionFunctions::CreateSelectAllMeshSelection(DynamicMesh, Selection);
						FGeometryScriptGroupLayer GL = FGeometryScriptGroupLayer();
						GL.bDefaultLayer = false;
						GL.ExtendedLayerIndex = 0;
						int32 OutPolyGroupID;
						UGeometryScriptLibrary_MeshPolygroupFunctions::SetPolygroupForMeshSelection(DynamicMesh, GL, Selection, OutPolyGroupID, (int32)GetWaterfallSettings()->SelectedMesh);

						//Make sure the paths are hidden now that there's mesh data if we're on the meshes tab (not the simple tab)
						if(GetWaterfallSettings()->GetCurrentTabState() == ESH_W2_TabState::W2_TS_Meshes) GetParentWaterfall()->SetPathsVisibility(false);
					}
				}

				if (TriCount > 0)
				{
					GetParentWaterfall()->MarkPackageDirty();
					USH_WaterfallTool2Statics::PrintEditorNotification("Mesh generation successful!");
				}
				else USH_WaterfallTool2Statics::PrintEditorNotification("Mesh generation failed! Tri count for mesh is 0!", ESH_W2_NotificationState::W2_NS_Fail);
			}
			else
			{
				USH_WaterfallTool2Statics::PrintEditorNotification("Mesh generation failed! Can't find correct mesh!", ESH_W2_NotificationState::W2_NS_Fail);
			}
		}
		else
		{
			USH_WaterfallTool2Statics::PrintEditorNotification("Mesh generation failed! No positions or triangles!", ESH_W2_NotificationState::W2_NS_Fail);
		}

		GetParentWaterfall()->RefreshDetailsPanel();
		ProcessingFinished();
	}
}

void FSH_RemappedWaterfallPath::Add(FSH_PositionPathPair NewPoint)
{
	Points.Add(NewPoint);
}

int32 FSH_RemappedWaterfallPath::Num() const
{
	return Points.Num();
}

FVector FSH_RemappedWaterfallPath::GetPosition(int32 Index) const
{
	return (Index >= 0 && Index < Points.Num()) ? Points[Index].GetPosition() : FVector::ZeroVector;
}

FVector FSH_RemappedWaterfallPath::GetNormal(int32 Index) const
{
	return (Index >= 0 && Index < Points.Num()) ? Points[Index].GetNormal() : FVector::ZeroVector;
}

FVector FSH_RemappedWaterfallPath::GetTangent(int32 Index) const
{
	return (Index >= 0 && Index < Points.Num()) ? Points[Index].GetTangent() : FVector::ZeroVector;
}

float FSH_RemappedWaterfallPath::GetDistance(int32 Index) const
{
	return (Index >= 0 && Index < Points.Num()) ? Points[Index].GetDistance() : 0.f;
}

float FSH_RemappedWaterfallPath::GetNormalisedDistance(int32 Index) const
{
	return (Index >= 0 && Index < Points.Num()) ? Points[Index].GetNormalisedDistance() : 0.f;
}

float FSH_RemappedWaterfallPath::GetTurbulence(int32 Index) const
{
	return (Index >= 0 && Index < Points.Num()) ? Points[Index].GetTurbulence() : 0.f;
}

float FSH_RemappedWaterfallPath::GetFlow(int32 Index) const
{
	return (Index >= 0 && Index < Points.Num()) ? Points[Index].GetFlow() : 0.f;
}

FVector FSH_RemappedWaterfallPath::GetVelocity(int32 Index) const
{
	return (Index >= 0 && Index < Points.Num()) ? Points[Index].GetVelocity() : FVector::ZeroVector;
}

FVector FSH_PositionPathPair::GetPosition() const
{
	if (OriginalPath)
	{
		if (PositionIndex >= 0 && PositionIndex < OriginalPath->PositionsCache.Num())
		{
			return OriginalPath->PositionsCache[PositionIndex];
		}
	}

	return FVector::ZeroVector;
}

FVector FSH_PositionPathPair::GetNormal() const
{
	if (OriginalPath)
	{
		if (PositionIndex >= 0 && PositionIndex < OriginalPath->NormalsCache.Num())
		{
			return OriginalPath->NormalsCache[PositionIndex];
		}
	}

	return FVector::ZeroVector;
}

FVector FSH_PositionPathPair::GetTangent() const
{
	if (OriginalPath)
	{
		if (PositionIndex >= 0 && PositionIndex < OriginalPath->TangentsCache.Num())
		{
			return OriginalPath->TangentsCache[PositionIndex];
		}
	}

	return FVector::ZeroVector;
}

float FSH_PositionPathPair::GetDistance() const
{
	if (OriginalPath)
	{
		if (PositionIndex >= 0 && PositionIndex < OriginalPath->DistanceCache.Num())
		{
			return OriginalPath->DistanceCache[PositionIndex];
		}
	}

	return 0.f;
}

float FSH_PositionPathPair::GetNormalisedDistance() const
{
	if (OriginalPath)
	{
		if (PositionIndex >= 0 && PositionIndex < OriginalPath->NormalisedDistanceCache.Num())
		{
			return OriginalPath->NormalisedDistanceCache[PositionIndex];
		}
	}

	return 0.f;
}

float FSH_PositionPathPair::GetTurbulence() const
{
	if (OriginalPath)
	{
		if (PositionIndex >= 0 && PositionIndex < OriginalPath->TurbulenceCache.Num())
		{
			return OriginalPath->TurbulenceCache[PositionIndex];
		}
	}

	return 0.f;
}

float FSH_PositionPathPair::GetFlow() const
{
	if (OriginalPath)
	{
		if (PositionIndex >= 0 && PositionIndex < OriginalPath->FlowCache.Num())
		{
			return OriginalPath->FlowCache[PositionIndex];
		}
	}

	return 0.f;
}

FVector FSH_PositionPathPair::GetVelocity() const
{
	if (OriginalPath)
	{
		if (PositionIndex >= 0 && PositionIndex < OriginalPath->VelocitiesCache.Num())
		{
			return OriginalPath->VelocitiesCache[PositionIndex];
		}
	}

	return FVector::ZeroVector;
}

bool FSH_PositionPathPair::LineSegmentIntersection(FVector2D line1point1, FVector2D line1point2, FVector2D line2point1, FVector2D line2point2)
{
	FVector2D a = line1point2 - line1point1;
	FVector2D b = line2point1 - line2point2;
	FVector2D c = line1point1 - line2point1;

	float alphaNumerator = b.Y * c.X - b.X * c.Y;
	float betaNumerator = a.X * c.Y - a.Y * c.X;
	float denominator = a.Y * b.X - a.X * b.Y;

	if (denominator == 0)
	{
		return false;
	}
	else if (denominator > 0)
	{
		if (alphaNumerator < 0 || alphaNumerator > denominator || betaNumerator < 0 || betaNumerator > denominator)
		{
			return false;
		}
	}
	else if (alphaNumerator > 0 || alphaNumerator < denominator || betaNumerator > 0 || betaNumerator < denominator)
	{
		return false;
	}
	return true;
}

bool FSH_PositionPathPairWithPath::operator<(const FSH_PositionPathPairWithPath b) const
{
	//If they intersect, A < B (return true), else return false
	FVector LineStartA = (PositionIndex == 0) ? RemappedPath.GetPosition(PositionIndex) : RemappedPath.GetPosition(PositionIndex - 1);
	FVector LineEndA = (PositionIndex == 0) ? RemappedPath.GetPosition(PositionIndex + 1) : RemappedPath.GetPosition(PositionIndex);

	//This needs to compare it to all the lines in the line because line A13-14 may not intersect B13-14 but it might intersect B15-16, especially if the segments are small
	bool bIntersecting = false;
	for (int i = 0; i < b.RemappedPath.Num() - 1; i++)
	{
		FVector LineStartB = b.RemappedPath.GetPosition(i);
		FVector LineEndB = b.RemappedPath.GetPosition(i + 1);

		bIntersecting = LineSegmentIntersection(
			FVector2D(LineStartA), FVector2D(LineEndA),
			FVector2D(LineStartB), FVector2D(LineEndB)
		);

		if (bIntersecting) break;
	}

	return bIntersecting;
}

TArray<FSH_RemappedWaterfallPath> FSH_WaterfallBuilder_Mesh::Singular_CovertToRemappedPaths()
{
	TArray<FSH_RemappedWaterfallPath> RemappedLines = {};

	//Check for inconsistencies in the path caches - they all need to be the same length for this to work
	bool bConsistent = true;
	int32 HighestNum = PathsToProcess[0]->PositionsCache.Num();
	for (int i = 1; i < PathsToProcess.Num(); i++)
	{
		if (PathsToProcess[i]->PositionsCache.Num() != HighestNum)
		{
			bConsistent = false;
			if (PathsToProcess[i]->PositionsCache.Num() > HighestNum) HighestNum = PathsToProcess[i]->PositionsCache.Num();
		}
	}

	if (!bConsistent)
	{
		for (int i = 0; i < PathsToProcess.Num(); i++)
		{
			PathsToProcess[i]->ResampleCache(HighestNum);
		}
	}

	FString PrintPoints = "";

	//Convert to path structs
	for (int i = 0; i < PathsToProcess.Num(); i++)
	{
		FSH_RemappedWaterfallPath NewLine = FSH_RemappedWaterfallPath(i);
		for (int j = 0; j < PathsToProcess[i]->PositionsCache.Num(); j++)
		{
			NewLine.Add(FSH_PositionPathPair(j, PathsToProcess[i]));
			if (!(i == 0 && j == 0)) PrintPoints += "\n";
			PrintPoints += FString::SanitizeFloat(PathsToProcess[i]->PositionsCache[j].X) + "," + FString::SanitizeFloat(PathsToProcess[i]->PositionsCache[j].Y);
		}

		RemappedLines.Add(NewLine);

		PrintPoints += "\n";
	}

	return RemappedLines;
}

TArray<FSH_RemappedWaterfallPath> FSH_WaterfallBuilder_Mesh::Singular_RemapPoints(bool bPrintPaths)
{
	TArray<FSH_RemappedWaterfallPath> RemappedLines = Singular_CovertToRemappedPaths();

	//Make into horizontal arrays (based on index)
	TArray<TArray<FSH_PositionPathPairWithPath>> PositionMapH = {};
	PositionMapH.Init({}, RemappedLines[0].Num());

	for (FSH_RemappedWaterfallPath line : RemappedLines)
	{
		for (int k = 0; k < line.Points.Num(); k++)
		{
			PositionMapH[k].Add(FSH_PositionPathPairWithPath(line.Points[k], line));
		}
	}

	//We need to do this multiple times to make sure it is all sorted properly
	//We also need to limit the loop to an odd number to make sure that we avoid endless loops
	//There is a possibility that if there are LOTS of points and LOTS of paths that eventually 99 won't be enough
	//But that's to come back and fix in a future update
	bool bSwap = true;
	int32 Count = 99;
	while (bSwap && Count > 0)
	{
		bSwap = false;
		Count--;

		//Sort the horizontal arrays
		for (int i = 1; i < PositionMapH.Num(); i++) //We don't sort row 0
		{
			TArray<FSH_PositionPathPairWithPath> SortedArray = { };
			for (int j = 0; j < PositionMapH[i].Num(); j++)
			{
				bool bPlaced = false;
				for (int k = 0; k < SortedArray.Num(); k++)
				{
					if (PositionMapH[i][j] < SortedArray[k])
					{
						SortedArray.Insert(PositionMapH[i][j], k);

						//Swap in the comparison lines
						for (int l = SortedArray[k].PositionIndex + 1; l < SortedArray[k].RemappedPath.Num(); l++)
						{
							FSH_PositionPathPair ThisPoint = SortedArray[k].RemappedPath.Points[l];
							FSH_PositionPathPair NextPoint = SortedArray[k + 1].RemappedPath.Points[l];
							SortedArray[k].RemappedPath.Points[l] = NextPoint;
							SortedArray[k + 1].RemappedPath.Points[l] = ThisPoint;
						}

						bPlaced = true;
						bSwap = true;
						break;
					}
				}

				if (!bPlaced) SortedArray.Add(PositionMapH[i][j]);
			}

			PositionMapH[i] = SortedArray;

			//Convert back into vertical paths
			TArray<TArray<FSH_PositionPathPairWithPath>> PositionMapV = {};
			for (int k = 0; k < PathsToProcess.Num(); k++)
			{
				TArray<FSH_PositionPathPairWithPath> NewPath = {};
				for (int j = 0; j < PositionMapH.Num(); j++)
				{
					NewPath.Add(PositionMapH[j][k]);
				}

				PositionMapV.Add(NewPath);
			}

			RemappedLines.Empty();
			//Convert each array into a new line for drawing
			for (int k = 0; k < PositionMapV.Num(); k++)
			{
				FSH_RemappedWaterfallPath NewLine = FSH_RemappedWaterfallPath(k);
				for (FSH_PositionPathPairWithPath Position : PositionMapV[k])
				{
					NewLine.Add(Position);
				}

				RemappedLines.Add(NewLine);
			}

			//Update the horizontal arrays (based on index)
			PositionMapH = {};
			PositionMapH.Init({}, RemappedLines[0].Num());

			for (FSH_RemappedWaterfallPath line : RemappedLines)
			{
				for (int k = 0; k < line.Points.Num(); k++)
				{
					PositionMapH[k].Add(FSH_PositionPathPairWithPath(line.Points[k], line));
				}
			}
		}
	}

	//One last thing, we need to sort all the points by z height so that we don't get lines zigzagging backwards up the waterfall
	for (int i = 0; i < RemappedLines.Num(); i++)
	{
		RemappedLines[i].Points.Sort([](const FSH_PositionPathPair& a, const FSH_PositionPathPair& b)
		{
			return b.GetPosition().Z < a.GetPosition().Z;
		});
	}

	//Print
	if (bPrintPaths)
	{
		for (int i = 0; i < RemappedLines.Num(); i++)
		{
			FColor Colour = FColor::Red;
			if (i % 10 == 0) Colour = FColor::Red;
			else if (i % 9 == 0) Colour = FColor::Green;
			else if (i % 8 == 0) Colour = FColor::Blue;
			else if (i % 7 == 0) Colour = FColor::Yellow;
			else if (i % 6 == 0) Colour = FColor::Cyan;
			else if (i % 5 == 0) Colour = FColor::Magenta;
			else if (i % 4 == 0) Colour = FColor::Orange;
			else if (i % 3 == 0) Colour = FColor::Purple;
			else if (i % 2 == 0) Colour = FColor::Turquoise;
			else Colour = FColor::Emerald;

			for (int j = 0; j < RemappedLines[i].Num() - 1; j++)
			{
				DrawDebugLine(GetWaterfallSettings()->GetWorld(), RemappedLines[i].GetPosition(j), RemappedLines[i].GetPosition(j + 1), Colour, false, 15.f, (uint8)'\000', 10.f);
				if (j == 0)
				{
					DrawDebugPoint(GetWaterfallSettings()->GetWorld(), RemappedLines[i].GetPosition(0), 20.f, Colour, false, 15.f, (uint8)'\000');
				}
				DrawDebugPoint(GetWaterfallSettings()->GetWorld(), RemappedLines[i].GetPosition(j + 1), 20.f, Colour, false, 15.f, (uint8)'\000');
			}
		}
	}

	return RemappedLines;
}

void FSH_WaterfallBuilder_Mesh::ApplySamplingSettings()
{
	for (USH_WaterfallPathComponent* WaterfallPath : PathsProcessing)
	{
		//if (ProcessingType == ESH_W2_ProcessingType::W2_PT_Mesh)
		//{
			if (GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_Singular && GetWaterfallSettings()->bSingularMeshSubdivide)
			{
				WaterfallPath->SetSampleLength(0.f, GetWaterfallSettings()->SingularMeshSubdivisions.Y, GetWaterfallSettings()->SingularMeshSubdivisions.Y);
			}
			else
			{
				WaterfallPath->SetSampleLength(GetWaterfallSettings()->SegmentLength, 2, GetWaterfallSettings()->MaxSegments);
			}

			if (GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_Singular)
			{
				WaterfallPath->SetSampleIncludeHits(false, 0.f);
			}
			else
			{
				WaterfallPath->SetSampleIncludeHits(GetWaterfallSettings()->bForceIncludeHits, GetWaterfallSettings()->ForceIncludeHitsTheshold);
			}
		//}
	}
}

void FSH_WaterfallBuilder_Mesh::ClearMeshBuffers()
{
	Positions.Empty();
	Normals.Empty();

	Colours.Empty();
	UVs0.Empty();
	UVs1.Empty();
	UVs2.Empty();
	UVs3.Empty();
	Triangles.Empty();
	FxData1.Empty();
	FxData2.Empty();

	VerticesIndexes.Empty();
}

void FSH_WaterfallBuilder_Mesh::Debug_ShowVertices(float Duration, FIntVector2 DebugPointsShowOnlyThesePoints)
{
	if (Positions.Num() > 0)
	{
		if (DebugPointsShowOnlyThesePoints.Y < 0) DebugPointsShowOnlyThesePoints.Y = Positions.Num() - 1;
		FIntVector2 Range = FIntVector2(FMath::Max(DebugPointsShowOnlyThesePoints.X, 0), FMath::Min(DebugPointsShowOnlyThesePoints.Y, Positions.Num() - 1));
		for (int i = FMath::Min(Range.X, Range.Y); i <= FMath::Max(Range.X, Range.Y); i++)
		{
			UKismetSystemLibrary::DrawDebugPoint(GetWaterfallSettings(), Positions[i], 20.f, FLinearColor::Red, Duration);
			UKismetSystemLibrary::DrawDebugString(GetWaterfallSettings(), Positions[i] + FVector(25.f, 0.f, 25.f), FString::SanitizeFloat(i, 0));
		}
	}
}

void DrawDebugTriangle(const UObject* WorldContextObject, FVector PointA, FVector PointB, FVector PointC, FColor Colour, float Thickness, bool bPersistent, float Lifetime)
{
	if (WorldContextObject)
	{
		DrawDebugLine(WorldContextObject->GetWorld(), PointA, PointB, Colour, bPersistent, Lifetime, (uint8)'\000', Thickness);
		DrawDebugLine(WorldContextObject->GetWorld(), PointA, PointC, Colour, bPersistent, Lifetime, (uint8)'\000', Thickness);
		DrawDebugLine(WorldContextObject->GetWorld(), PointB, PointC, Colour, bPersistent, Lifetime, (uint8)'\000', Thickness);
	}
}

void FSH_WaterfallBuilder_Mesh::Debug_ShowTriangles(float Duration)
{
	for (int i = 0; i < Triangles.Num(); i++)
	{
		DrawDebugTriangle(GetParentWaterfall(), Positions[Triangles[i].X], Positions[Triangles[i].Y], Positions[Triangles[i].Z], FColor::Cyan, 4.f, false, Duration);
	}
}

void FSH_WaterfallBuilder_Mesh::Debug_ShowDirections(float Duration)
{
	TArray<USH_WaterfallPathComponent*> WaterfallPaths = GetParentWaterfall()->GetAllPathComponents();

	for (USH_WaterfallPathComponent* CurrentPath : WaterfallPaths)
	{
		for (int i = 0; i < CurrentPath->PositionsCache.Num(); i++)
		{
			UKismetSystemLibrary::DrawDebugArrow(GetWaterfallSettings(), CurrentPath->PositionsCache[i], CurrentPath->PositionsCache[i] + (CurrentPath->GetPointDirection(i) * FVector(20.f)), 20.f, FLinearColor::Green, Duration);
		}
	}
}

void FSH_WaterfallBuilder_Mesh::Debug_PrintRemappedPaths()
{
	PathsToProcess = GetParentWaterfall()->GetAllPathComponents();
	for (int i = 0; i < PathsToProcess.Num(); i++)
	{
		PathsToProcess[i]->CachePath();
	}

	Singular_RemapPoints(true);
	PathsToProcess.Empty();
}

FVector SplineInterpolation(FVector A, FVector B, FVector C, FVector D, float T)
{
	FVector A2 = A * FMath::Pow(1.f - T, 3.f);
	FVector B2 = B * 3.f * T * FMath::Square(1.f - T);
	FVector C2 = C * 3.f * FMath::Square(T) * (1.f - T);
	FVector D2 = D * FMath::Pow(T, 3.f);

	return A2 + B2 + C2 + D2;
}

void FSH_WaterfallBuilder_Mesh::BuildMeshBuffers_SingularRemesh(FSH_RemappedWaterfallPath CurrentPath, FSH_RemappedWaterfallPath NextPath, float NextPathInterp, float PathAlpha, int32 PathIndex)
{
	if (CurrentPath.Num() - 1 == NextPath.Num() - 1)
	{
		for (int i = 0; i < CurrentPath.Num(); i++)
		{
			FOS_PathValues Values = FOS_PathValues(CurrentPath, NextPath, i, NextPathInterp, GetWaterfallSettings());

			//1. Calculate Position
			{
				float TangentOffset =
					(GetWaterfallSettings()->SingularMeshAdditionalGeoSpreadCurve)
					? FMath::Lerp(0.f, GetWaterfallSettings()->GetSingularMeshAdditionalGeo() * -1.f, GetWaterfallSettings()->SingularMeshAdditionalGeoSpreadCurve->GetFloatValue(PathAlpha))
					: FMath::Lerp(GetWaterfallSettings()->GetSingularMeshAdditionalGeo(), GetWaterfallSettings()->GetSingularMeshAdditionalGeo() * -1.f, PathAlpha);

				if (GetWaterfallSettings()->SingularMeshAdditionalGeoCurve) TangentOffset *= GetWaterfallSettings()->SingularMeshAdditionalGeoCurve->GetFloatValue(Values.NormalisedDistance);

				//Potentially project the vertices to make sure they don't go below the ground and/or are against a surface
				float TraceAlpha = (GetWaterfallSettings()->MeshTraceAlpha) ? GetWaterfallSettings()->MeshTraceAlpha->GetFloatValue(Values.NormalisedDistance) : 1.f;

				Positions.Add(
					GetProjectedPosition(
						Values.Position + (Values.Tangent * TangentOffset),
						Values.Normal,
						GetWaterfallSettings()->MeshTraceDistance,
						GetWaterfallSettings()->MeshTraceOffset,
						TraceAlpha,
						Values.bFlattenLastIndex,
						GetWaterfallSettings()->bMeshTraceProjection,
						GetWaterfallSettings()->MeshTraceMode
					));
			}

			//2. Calculate Normal
			MeshBuffers_CalculateNormals(Normals, Values, PathAlpha, CurrentPath.Points[i].OriginalPath);

			//3. Calculate Colour
			MeshBuffers_CalculateColour(Values, Values.Velocity.GetSafeNormal(0.0001f), 1);

			//4. Calculate UVs
			{
				UVs0.Add(GetWaterfallSettings()->BaseUVsScale * FVector2D(PathAlpha, Values.NormalisedDistance));
				UVs1.Add(GetWaterfallSettings()->BaseUVsScale * FVector2D(FMath::Lerp(CurrentPath.Points[i].OriginalPath->SplineDistance, NextPath.Points[i].OriginalPath->SplineDistance, NextPathInterp), Values.Distance));
				MeshBuffers_CalculateUV2(Values, GetWaterfallSettings()->BaseUVsScale.Y, 1);
				UVs3.Add(FVector2D(FMath::Lerp(CurrentPath.Points[i].OriginalPath->UVSeed, NextPath.Points[i].OriginalPath->UVSeed, NextPathInterp), FMath::Lerp(CurrentPath.Points[i].OriginalPath->NormalisedSplineDistance, NextPath.Points[i].OriginalPath->NormalisedSplineDistance, NextPathInterp)));
			}

			//5. Build quad data but don't add triangles for end vertices
			if (NumPathsProcessed < GetWaterfallSettings()->SingularMeshSubdivisions.X - 1 && i < GetWaterfallSettings()->SingularMeshSubdivisions.Y - 1)
			{
				/*Build two triangles to make a quad*/
				int32 A = Positions.Num() - 1;
				int32 B = A + 1;
				int32 C = A + GetWaterfallSettings()->SingularMeshSubdivisions.Y;
				int32 D = C + 1;

				bool bFN = GetWaterfallSettings()->bFlipNormals;
				FIntVector TriangleABC = FIntVector((bFN) ? C : A, B, (bFN) ? A : C);
				FIntVector TriangleCBD = FIntVector((bFN) ? D : C, B, (bFN) ? C : D);

				Triangles.Add(TriangleABC);
				Triangles.Add(TriangleCBD);
			}
		}
	}
}

void FSH_WaterfallBuilder_Mesh::BuildMeshBuffers_SingularNoRemesh(FSH_RemappedWaterfallPath CurrentPath, FSH_RemappedWaterfallPath NextPath)
{
	int32 IndexOffset = FMath::Max(Positions.Num() - CurrentPath.Num(), 0);
	int32 CurrentPathVertexIndex = 0;
	int32 NextPathVertexIndex = 0;
	bool bNextVertexOnCurrentPath = true;
	bool bNextVertexOnNextPath = true;

	bool bCurrent = false;
	TArray<int32> TriangleIDs = {};

	auto ProcessPoints = [&](FSH_RemappedWaterfallPath Path, int32 LocalIndex)
	{
		//Convert the local index to a global index
		int32 GlobalIndex = LocalIndex + IndexOffset;
		if (!bCurrent) GlobalIndex += CurrentPath.Num();

		//Don't recreate data if it's already been created previously
		if (VerticesIndexes.Contains(GlobalIndex))
		{
			TriangleIDs.Add(*VerticesIndexes.Find(GlobalIndex));
		}
		else
		{
			FOS_PathValues Values = FOS_PathValues(Path, LocalIndex);
			bool bCurrentTest = (bCurrent) ? bNextVertexOnCurrentPath : bNextVertexOnNextPath;
			Values.bFlattenLastIndex = Path.Points[LocalIndex].OriginalPath->PathTerminatesInWater() && !bCurrentTest;

			float PathAlpha = (float)Path.RemappedArrayIndex / (float)FMath::Max(PathsProcessing.Num() - 1, 1);

			//1. Calculate Position
			{
				float TangentOffset =
					(GetWaterfallSettings()->SingularMeshAdditionalGeoSpreadCurve)
					? FMath::Lerp(0.f, GetWaterfallSettings()->GetSingularMeshAdditionalGeo() * -1.f, GetWaterfallSettings()->SingularMeshAdditionalGeoSpreadCurve->GetFloatValue(PathAlpha))
					: FMath::Lerp(GetWaterfallSettings()->GetSingularMeshAdditionalGeo(), GetWaterfallSettings()->GetSingularMeshAdditionalGeo() * -1.f, PathAlpha);

				if (GetWaterfallSettings()->SingularMeshAdditionalGeoCurve) TangentOffset *= GetWaterfallSettings()->SingularMeshAdditionalGeoCurve->GetFloatValue(Values.NormalisedDistance);

				//Potentially project the vertices to make sure they don't go below the ground and/or are against a surface
				float TraceAlpha = (GetWaterfallSettings()->MeshTraceAlpha) ? GetWaterfallSettings()->MeshTraceAlpha->GetFloatValue(Values.NormalisedDistance) : 1.f;

				int32 NewIndex = Positions.Add(
					GetProjectedPosition(
						Values.Position + (Values.Tangent * TangentOffset),
						Values.Normal,
						GetWaterfallSettings()->MeshTraceDistance,
						GetWaterfallSettings()->MeshTraceOffset,
						TraceAlpha,
						Values.bFlattenLastIndex,
						GetWaterfallSettings()->bMeshTraceProjection,
						GetWaterfallSettings()->MeshTraceMode
					));

				VerticesIndexes.Add(GlobalIndex, NewIndex);
				TriangleIDs.Add(NewIndex);
			}

			//2. Calculate Normal
			MeshBuffers_CalculateNormals(Normals, Values, PathAlpha, Path.Points[LocalIndex].OriginalPath);

			//3. Calculate Colour
			MeshBuffers_CalculateColour(Values, Values.Velocity.GetSafeNormal(0.0001f), 1);

			//4. Calculate UVs
			{
				UVs0.Add(GetWaterfallSettings()->BaseUVsScale * FVector2D(PathAlpha, Values.NormalisedDistance));
				UVs1.Add(GetWaterfallSettings()->BaseUVsScale * FVector2D(Path.Points[LocalIndex].OriginalPath->SplineDistance, Values.Distance));
				MeshBuffers_CalculateUV2(Values, GetWaterfallSettings()->BaseUVsScale.Y, 1);
				MeshBuffers_CalculateUV3(Values, Path.Points[LocalIndex].OriginalPath, 1);
			}
		}
	};

	auto BuildTriangle = [&]()
	{
		//Reverse order for second triangle
		if (!bCurrent) Algo::Reverse(TriangleIDs);

		//Reverse order to flip normals
		if (GetWaterfallSettings()->bFlipNormals) Algo::Reverse(TriangleIDs);

		Triangles.Add(FIntVector(TriangleIDs[0], TriangleIDs[1], TriangleIDs[2]));
		TriangleIDs.Empty();
	};

	//While there are valid triangles connecting vertices between the two current focused paths
	while (bNextVertexOnCurrentPath || bNextVertexOnNextPath)
	{
		if (bNextVertexOnCurrentPath)
		{
			if (CurrentPathVertexIndex + 1 >= 0 && CurrentPathVertexIndex + 1 < CurrentPath.Num()
				&& NextPathVertexIndex >= 0 && NextPathVertexIndex < NextPath.Num())
			{
				bCurrent = true;
				ProcessPoints(CurrentPath, CurrentPathVertexIndex);

				CurrentPathVertexIndex++;
				ProcessPoints(CurrentPath, CurrentPathVertexIndex);

				bCurrent = false;
				ProcessPoints(NextPath, NextPathVertexIndex);

				bCurrent = true;
				BuildTriangle();
			}
			else
			{
				bNextVertexOnCurrentPath = false;
			}
		}

		if (bNextVertexOnNextPath)
		{
			if (NextPathVertexIndex + 1 >= 0 && NextPathVertexIndex + 1 < NextPath.Num()
				&& CurrentPathVertexIndex >= 0 && CurrentPathVertexIndex < CurrentPath.Num())
			{
				bCurrent = false;
				ProcessPoints(NextPath, NextPathVertexIndex);

				NextPathVertexIndex++;
				ProcessPoints(NextPath, NextPathVertexIndex);

				bCurrent = true;
				ProcessPoints(CurrentPath, CurrentPathVertexIndex);

				bCurrent = false;
				BuildTriangle();
			}
			else
			{
				bNextVertexOnNextPath = false;
			}
		}
	}
}

FSH_WaterfallBuilder_Mesh::FOS_PathValues::FOS_PathValues(USH_WaterfallPathComponent* CurrentPath, int32 i, USH_WaterfallSettingsComponent* WaterfallSettings)
{
	Index = i;

	bFlattenLastIndex = (i == CurrentPath->PositionsCache.Num() - 1 && CurrentPath->PathTerminatesInWater());

	Position = CurrentPath->PositionsCache[i];
	Normal = CurrentPath->NormalsCache[i];
	Tangent = CurrentPath->TangentsCache[i];
	Distance = CurrentPath->DistanceCache[i];
	//In the splash mesh the normalised distance will always be 1 because we're always working on the last path index
	NormalisedDistance = CurrentPath->NormalisedDistanceCache[i];
	Turbulence = CurrentPath->TurbulenceCache[i];
	Flow = CurrentPath->FlowCache[i];
	Velocity = CurrentPath->VelocitiesCache[i];
	Speed = Velocity.Size();

	Width = WaterfallSettings->PerPathWidth;
	if (WaterfallSettings->PerPathWidthCurve) Width *= WaterfallSettings->PerPathWidthCurve->GetFloatValue(NormalisedDistance);
}

FSH_WaterfallBuilder_Mesh::FOS_PathValues::FOS_PathValues(FSH_RemappedWaterfallPath CurrentPath, int32 i)
{
	Index = i;

	Position = CurrentPath.GetPosition(i);
	Normal = CurrentPath.GetNormal(i);
	Tangent = CurrentPath.GetTangent(i);
	Distance = CurrentPath.GetDistance(i);
	NormalisedDistance = CurrentPath.GetNormalisedDistance(i);
	Turbulence = CurrentPath.GetTurbulence(i);
	Flow = CurrentPath.GetFlow(i);
	Velocity = CurrentPath.GetVelocity(i);
	Speed = Velocity.Size();
}

FSH_WaterfallBuilder_Mesh::FOS_PathValues::FOS_PathValues(FSH_RemappedWaterfallPath CurrentPath, FSH_RemappedWaterfallPath NextPath, int32 i, float NextPathInterp, USH_WaterfallSettingsComponent* WaterfallSettings)
{
	Index = i;

	bFlattenLastIndex = (i == CurrentPath.Num() - 1 && NextPath.Points[i].OriginalPath->PathTerminatesInWater());

	if (WaterfallSettings->SingularMeshSubdivisionSplineInterpolation == ESH_W2_Interpolation::W2_IN_Cubic)
	{
		float DistanceWithInterp = FVector::Dist(CurrentPath.GetPosition(i), NextPath.GetPosition(i)) * 0.3333f; //0.3333f makes the interpolation cubic

		Position = SplineInterpolation(
			CurrentPath.GetPosition(i),
			CurrentPath.GetPosition(i) - (CurrentPath.GetTangent(i) * DistanceWithInterp),
			NextPath.GetPosition(i) + (NextPath.GetTangent(i) * DistanceWithInterp),
			NextPath.GetPosition(i),
			NextPathInterp);
	}
	else
	{
		Position = FMath::Lerp(CurrentPath.GetPosition(i), NextPath.GetPosition(i), NextPathInterp);
	}

	//Lerp the cached values to find the point between two points
	Normal = USH_WaterfallTool2Statics::SlerpNormals(CurrentPath.GetNormal(i), NextPath.GetNormal(i), NextPathInterp);
	Tangent = USH_WaterfallTool2Statics::SlerpNormals(CurrentPath.GetTangent(i), NextPath.GetTangent(i), NextPathInterp);
	Turbulence = FMath::Lerp(CurrentPath.GetTurbulence(i), NextPath.GetTurbulence(i), NextPathInterp);
	NormalisedDistance = FMath::Lerp(CurrentPath.GetNormalisedDistance(i), NextPath.GetNormalisedDistance(i), NextPathInterp);
	Distance = FMath::Lerp(CurrentPath.GetDistance(i), NextPath.GetDistance(i), NextPathInterp);
	Flow = FMath::Lerp(CurrentPath.GetFlow(i), NextPath.GetFlow(i), NextPathInterp);
	Velocity = FMath::Lerp(CurrentPath.GetVelocity(i), NextPath.GetVelocity(i), NextPathInterp);
	Speed = Velocity.Size();
}

void FSH_WaterfallBuilder_Mesh::MeshBuffers_CalculatePositions(TArray<FVector>& PositionsArray, FOS_PathValues& Values, int32 Subdivisions, FVector OffsetVector)
{
	FVector LeftVertexPosition = Values.Position - OffsetVector;
	FVector RightVertexPosition = Values.Position + OffsetVector;

	//Potentially project the vertices to make sure they don't go below the ground and/or are against a surface
	if (GetWaterfallSettings()->MeshTraceDistance > 0.f)
	{
		float TraceAlpha = (GetWaterfallSettings()->MeshTraceAlpha) ? GetWaterfallSettings()->MeshTraceAlpha->GetFloatValue(Values.NormalisedDistance) : 1.f;

		LeftVertexPosition = GetProjectedPosition(
			LeftVertexPosition,
			Values.Normal,
			GetWaterfallSettings()->MeshTraceDistance,
			GetWaterfallSettings()->MeshTraceOffset,
			TraceAlpha,
			Values.bFlattenLastIndex,
			GetWaterfallSettings()->bMeshTraceProjection,
			GetWaterfallSettings()->MeshTraceMode
		);

		RightVertexPosition = GetProjectedPosition(
			RightVertexPosition,
			Values.Normal,
			GetWaterfallSettings()->MeshTraceDistance,
			GetWaterfallSettings()->MeshTraceOffset,
			TraceAlpha,
			Values.bFlattenLastIndex,
			GetWaterfallSettings()->bMeshTraceProjection,
			GetWaterfallSettings()->MeshTraceMode
		);
	}

	//Calculate bulge here for Cross mesh
	if (GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_Cross)
	{
		Values.Position = FMath::Lerp(LeftVertexPosition, RightVertexPosition, 0.5f); //This is necessary because we're moving the position to be in the middle of the projected positions

		if (GetWaterfallSettings()->PerPathBulge != 0.f)
		{
			float BulgeLenthFade = (GetWaterfallSettings()->PerPathBulgeCurve) ? GetWaterfallSettings()->PerPathBulgeCurve->GetFloatValue(Values.NormalisedDistance) : 1.f;
			float BulgeProfileFade = (GetWaterfallSettings()->PerPathBulgeProfileCurve) ? GetWaterfallSettings()->PerPathBulgeProfileCurve->GetFloatValue(0.5f) : 1.f;
			FVector CorrectNormal = Values.bFlattenLastIndex ? Values.Normal.GetSafeNormal(0.001f) : Values.Normal;
			Values.Position += BulgeProfileFade * BulgeLenthFade * GetWaterfallSettings()->PerPathBulge * CorrectNormal;
		}

		//Cross plane width and offset
		Values.Width = GetWaterfallSettings()->CrossPlaneWidth;
		if (GetWaterfallSettings()->CrossPlaneWidthCurve) Values.Width *= GetWaterfallSettings()->CrossPlaneWidthCurve->GetFloatValue(Values.NormalisedDistance);

		float Offset = GetWaterfallSettings()->CrossPlaneOffset;
		if (GetWaterfallSettings()->CrossPlaneOffsetCurve) Offset *= GetWaterfallSettings()->CrossPlaneOffsetCurve->GetFloatValue(Values.NormalisedDistance);

		FVector A = (Values.Index == 0) ? USH_WaterfallTool2Statics::SlerpNormals(FVector(0.f, 0.f, 1.f), Values.Normal, GetWaterfallSettings()->CrossPlaneSplineRoll) : Values.Normal;
		FVector B = (Values.bFlattenLastIndex) ? A.GetSafeNormal() : A;
		FVector VertexOffset = Values.Width * B;

		//Calculate the left and right positions with the offset
		LeftVertexPosition = Values.Position + VertexOffset + (Offset * VertexOffset);
		RightVertexPosition = (Values.Position - VertexOffset) + (Offset * VertexOffset);
	}

	//Add left vertex
	PositionsArray.Add(LeftVertexPosition);

	//Add in-between vertexes based on subdivisions
	for (int j = 0; j < Subdivisions; j++)
	{
		float SubdivisionAlpha = (float)(j + 1) / (float)(Subdivisions + 1);

		// Calculate bulge here for PerPath and Splash mesh
		if (GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_PerPath
			|| GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_Splash)
		{
			FVector InterpVertexPos = FMath::Lerp(LeftVertexPosition, RightVertexPosition, SubdivisionAlpha);

			//Apply any bulge across the middle vertices
			//Flatten the first row if desired and if the curves don't exist 
			if (GetWaterfallSettings()->PerPathBulge == 0.f
				|| (Values.Index == 0
					&& GetWaterfallSettings()->bPerPathFlattenFirstRowBulge
					&& !(GetWaterfallSettings()->PerPathBulgeCurve || GetWaterfallSettings()->PerPathBulgeProfileCurve)
					)
				)
			{
				PositionsArray.Add(InterpVertexPos);
			}
			else
			{
				float BulgeLenthFade = (GetWaterfallSettings()->PerPathBulgeCurve) ? GetWaterfallSettings()->PerPathBulgeCurve->GetFloatValue(Values.NormalisedDistance) : 1.f;
				//Default to bilinear gradient if there's no curve
				float BulgeProfileFade = (GetWaterfallSettings()->PerPathBulgeProfileCurve) ? GetWaterfallSettings()->PerPathBulgeProfileCurve->GetFloatValue(SubdivisionAlpha) : 1.f - (FMath::Abs(SubdivisionAlpha - 0.5f) * 2.f);
				FVector CorrectNormal = Values.bFlattenLastIndex ? Values.Normal.GetSafeNormal(0.0001f) : Values.Normal;
				PositionsArray.Add(InterpVertexPos + ((BulgeProfileFade * BulgeLenthFade * GetWaterfallSettings()->PerPathBulge) * CorrectNormal));
			}
		}
		else
		{
			PositionsArray.Add(FMath::Lerp(LeftVertexPosition, RightVertexPosition, SubdivisionAlpha));
		}
	}

	//Add right vertex
	PositionsArray.Add(RightVertexPosition);
}

void FSH_WaterfallBuilder_Mesh::MeshBuffers_CalculateNormals(TArray<FVector>& NormalsArray, FOS_PathValues& Values, float PathAlpha, USH_WaterfallPathComponent* CurrentOriginalPath)
{
	//Default to bilinear gradient if there's no curve
	float SlerpAlpha = GetWaterfallSettings()->CylinderNormal * -1.f;
	SlerpAlpha *= (GetWaterfallSettings()->CylinderNormalCurve) ? GetWaterfallSettings()->CylinderNormalCurve->GetFloatValue(PathAlpha) : (PathAlpha - 0.5f) * 2.f;

	Values.Normal = USH_WaterfallTool2Statics::SlerpNormals(Values.Normal, Values.Tangent, SlerpAlpha);

	FVector FinalNormal = USH_WaterfallTool2Statics::SlerpNormals(Values.Normal, GetWaterfallSettings()->OverrideNormalDirection, GetWaterfallSettings()->OverrideNormal);
	if (CurrentOriginalPath->PathTerminatesInWater())
	{
		FinalNormal = USH_WaterfallTool2Statics::SlerpNormals(FinalNormal, FVector(0.f, 0.f, 1.f), FMath::GetMappedRangeValueClamped<float>(FFloatRange(0.f, GetWaterfallSettings()->ForceWaterNormalDistZ), FFloatRange(1.f, 0.f), Values.Position.Z - CurrentOriginalPath->GetSimulationWaterLevel()));
	}

	NormalsArray.Add(FinalNormal);
}

void FSH_WaterfallBuilder_Mesh::MeshBuffers_CalculateNormals(TArray<FVector>& NormalsArray, const FOS_PathValues& Values, int32 Subdivisions)
{
	auto AddCalculatedNormal = [&](float SlerpAlpha)
	{
		FVector FinalNormal = (GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_Cross)
			? USH_WaterfallTool2Statics::SlerpNormals(Values.Tangent, Values.Normal, SlerpAlpha * GetWaterfallSettings()->CylinderNormal * -1.f)
			: USH_WaterfallTool2Statics::SlerpNormals(Values.Normal, Values.Tangent, SlerpAlpha * GetWaterfallSettings()->CylinderNormal)
			;

		if (GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_Cross) FinalNormal = USH_WaterfallTool2Statics::SlerpNormals(FinalNormal, Values.Normal, GetWaterfallSettings()->CrossPlaneAlignNormal);
		FinalNormal = USH_WaterfallTool2Statics::SlerpNormals(FinalNormal, GetWaterfallSettings()->OverrideNormalDirection, GetWaterfallSettings()->OverrideNormal);

		NormalsArray.Add(FinalNormal);
	};

	//Left
	AddCalculatedNormal(GetWaterfallSettings()->CylinderNormalCurve ? GetWaterfallSettings()->CylinderNormalCurve->GetFloatValue(0.f) : -1.f);

	//Middle
	for (int j = 0; j < Subdivisions; j++)
	{
		float SubdivisionAlpha = (float)(j + 1) / (float)(Subdivisions + 1);

		AddCalculatedNormal(GetWaterfallSettings()->CylinderNormalCurve ? GetWaterfallSettings()->CylinderNormalCurve->GetFloatValue(SubdivisionAlpha) : (SubdivisionAlpha - 0.5f) * 2.f);
	}

	//Right
	AddCalculatedNormal(GetWaterfallSettings()->CylinderNormalCurve ? GetWaterfallSettings()->CylinderNormalCurve->GetFloatValue(1.f) : 1.f);

}

void FSH_WaterfallBuilder_Mesh::MeshBuffers_CalculateColour(const FOS_PathValues& Values, FVector Direction, int32 NumToAdd)
{
	FVector Colour = (Direction + 1) * 0.5f;
	FLinearColor FinalColour = FLinearColor(Colour.X, Colour.Y, Colour.Z, Values.Turbulence);

	for (int j = 0; j < NumToAdd; j++)
	{
		Colours.Add(FinalColour);
	}
}

void FSH_WaterfallBuilder_Mesh::MeshBuffers_CalculateUV2(const FOS_PathValues& Values, float YScale, int32 NumToAdd)
{
	FVector2D UV2 = FVector2D(Values.Speed, Values.Flow * YScale);

	for (int j = 0; j < NumToAdd; j++)
	{
		UVs2.Add(UV2);
	}
}

void FSH_WaterfallBuilder_Mesh::MeshBuffers_CalculateUV3(const FOS_PathValues& Values, USH_WaterfallPathComponent* CurrentPath, int32 NumToAdd)
{
	FVector2D UV3 = FVector2D(CurrentPath->UVSeed, CurrentPath->NormalisedSplineDistance);

	for (int j = 0; j < NumToAdd; j++)
	{
		UVs3.Add(UV3);
	}
}

void FSH_WaterfallBuilder_Mesh::BuildMeshBuffers_OnePath(USH_WaterfallPathComponent* CurrentPath, int32 Subdivisions)
{
	if (!CurrentPath->IsPathCacheConsistent()) return;

	for (int i = 0; i < CurrentPath->PositionsCache.Num(); i++)
	{
		FOS_PathValues Values = FOS_PathValues(CurrentPath, i, GetWaterfallSettings());

		//1. Calculate Position
		{
			//Figure out the left and right vertex positions - if it's the per path mesh we want to slerp but if it's the cross mesh, we want to lerp
			FVector SlerpVector = (GetWaterfallSettings()->SelectedMesh == ESH_MeshGenerationType::W2_MG_PerPath)
				? USH_WaterfallTool2Statics::SlerpNormals(Values.Tangent * FVector(1.f, 1.f, 0.f), Values.Tangent, GetWaterfallSettings()->PerPathSplineRoll) * Values.Width * 0.5f
				: FMath::Lerp(Values.Tangent * FVector(1.f, 1.f, 0.f), Values.Tangent, GetWaterfallSettings()->PerPathSplineRoll) * Values.Width * 0.5f;

			MeshBuffers_CalculatePositions(Positions, Values, Subdivisions, SlerpVector);
		}

		//2. Calculate Normal
		MeshBuffers_CalculateNormals(Normals, Values, Subdivisions);

		//3. Calculate colour
		MeshBuffers_CalculateColour(Values, Values.Velocity.GetSafeNormal(0.0001f), Subdivisions + 2); //Num Subdivisions + Left + Right

		//4. Calculate UVs
		{
			//UV0
			{
				auto AddUV0 = [&](float X)
				{
					UVs0.Add(GetWaterfallSettings()->BaseUVsScale * FVector2D(X, Values.NormalisedDistance));
				};

				//Left
				AddUV0(0.f);

				//Middle
				for (int j = 0; j < Subdivisions; j++)
				{
					float SubdivisionAlpha = (float)(j + 1) / (float)(Subdivisions + 1);
					AddUV0(FMath::Lerp(0.f, 1.f, SubdivisionAlpha));
				}

				//Right
				AddUV0(1.f);
			}

			//UV1
			{
				auto AddUV1 = [&](float X)
				{
					UVs1.Add(GetWaterfallSettings()->BaseUVsScale * FVector2D(X, Values.Distance));
				};

				//Left
				AddUV1(Values.Width * -0.5f);

				//Middle
				for (int j = 0; j < Subdivisions; j++)
				{
					float SubdivisionAlpha = (float)(j + 1) / (float)(Subdivisions + 1);
					AddUV1(FMath::Lerp(Values.Width * -0.5f, Values.Width * 0.5f, SubdivisionAlpha));
				}

				//Right
				AddUV1(Values.Width * 0.5f);
			}

			MeshBuffers_CalculateUV2(Values, GetWaterfallSettings()->BaseUVsScale.Y, Subdivisions + 2);
			MeshBuffers_CalculateUV3(Values, CurrentPath, Subdivisions + 2);
		}

		//5. Finalise Triangles
		{
			//Don't process the last line of vertices because there's nothing more to connect them to!
			if (i < CurrentPath->PositionsCache.Num() - 1)
			{
				for (int j = 0; j <= Subdivisions; j++)
				{
					/*Build two triangles to make a quad*/
					int32 A = j + (Positions.Num() - 1 - (Subdivisions + 1));
					int32 B = A + 1;
					int32 C = A + (Subdivisions + 2);
					int32 D = B + (Subdivisions + 2);

					bool bFN = GetWaterfallSettings()->bFlipNormals;
					FIntVector TriangleABC = FIntVector((bFN) ? C : A, B, (bFN) ? A : C);
					FIntVector TriangleCBD = FIntVector((bFN) ? B : D, C, (bFN) ? D : B);

					Triangles.Add(TriangleABC);
					Triangles.Add(TriangleCBD);
				}
			}
		}
	}
}

void FSH_WaterfallBuilder_Mesh::BuildMeshBuffers_Splash(USH_WaterfallPathComponent* CurrentPath)
{
	if (!CurrentPath->IsPathCacheConsistent()) return;

	int i = CurrentPath->PositionsCache.Num() - 1;

	FOS_PathValues Values = FOS_PathValues(CurrentPath, i, GetWaterfallSettings());
	Values.Position += FVector(0.f, 0.f, CurrentPath->UVSeed + 0.025f); //Nudge the splash mesh up and away from the water slightly

	//1. Calculate Positions
	TArray<FVector> WorkingPositions = {};
	{
		//Figure out the left and right vertex positions
		FVector TangentVector = Values.Tangent * Values.Width * 0.5f;
		MeshBuffers_CalculatePositions(WorkingPositions, Values, GetWaterfallSettings()->SplashMeshOverrideSubdivisions, TangentVector);
	}

	//2. Calculate Normals
	TArray<FVector> WorkingNormals = {};
	{
		MeshBuffers_CalculateNormals(WorkingNormals, Values, GetWaterfallSettings()->SplashMeshOverrideSubdivisions);
	}

	//3. Generate Splash Path
	TArray<FVector> CalculatedPositions = {};
	TArray<FVector> CalculatedDirections = {};
	TArray<float> CalculatedRadii = {};
	TArray<FVector> CalculatedNormals = {};
	TArray<FVector> LocalDirections = {};
	TArray<float> LocalDistances = {};
	{
		FVector ScaleOrigin = UKismetMathLibrary::GetVectorArrayAverage(WorkingPositions);

		//Start by figuring out the total distance of the outer mesh path
		float TotalDistance = 0.f;
		for (int j = 1; j < WorkingPositions.Num(); j++) //Skip index 0
		{
			TotalDistance += FVector::Dist(WorkingPositions[j], WorkingPositions[j] - 1);
		}

		//Construct the front of the mesh
		float MarchedDistance = 0.f;
		for (int j = 0; j < WorkingPositions.Num(); j++)
		{
			CalculatedPositions.Add(WorkingPositions[j]); //Add the position
			CalculatedNormals.Add(WorkingNormals[j]); //Add the normal
			LocalDirections.Add(FVector(0.f, -1.f, 0.f)); //Add the direction

			if (j > 0) MarchedDistance += FVector::Dist(WorkingPositions[j], WorkingPositions[j - 1]);
			LocalDistances.Add(MarchedDistance - (TotalDistance * 0.5f));

			CalculatedRadii.Add(GetWaterfallSettings()->SplashFrontRadius * GetWaterfallSettings()->SplashScale);

			FVector Dir = FVector::ZeroVector;
			if (j > 0 && j < WorkingPositions.Num() - 1) //First and last are slightly different
			{
				Dir = USH_WaterfallTool2Statics::SlerpNormals(
					UKismetMathLibrary::GetDirectionUnitVector(WorkingPositions[j], WorkingPositions[j - 1]).Cross(FVector(0.f, 0.f, -1.f)),
					UKismetMathLibrary::GetDirectionUnitVector(WorkingPositions[j + 1], WorkingPositions[j]).Cross(FVector(0.f, 0.f, -1.f)),
					0.5f);
			}
			else if (j > 0)
			{
				Dir = UKismetMathLibrary::GetDirectionUnitVector(WorkingPositions[j], WorkingPositions[j - 1]).Cross(FVector(0.f, 0.f, -1.f));
			}
			else
			{
				Dir = UKismetMathLibrary::GetDirectionUnitVector(WorkingPositions[j + 1], WorkingPositions[j]).Cross(FVector(0.f, 0.f, -1.f));
			}

			CalculatedDirections.Add(Dir);
		}

		//Rotate around the last point to create the end of the mesh
		for (int j = 0; j < GetWaterfallSettings()->SplashRadiusSubdivisions; j++)
		{
			CalculatedPositions.Add(WorkingPositions.Last());
			CalculatedNormals.Add(WorkingNormals.Last());

			float RotationAngleDegrees = 180.f / (FMath::Max(GetWaterfallSettings()->SplashRadiusSubdivisions - 1, 1) + 2);

			FVector Dir = CalculatedDirections.Last().RotateAngleAxis(RotationAngleDegrees, FVector(0.f, 0.f, 1.f).GetSafeNormal());
			CalculatedDirections.Add(Dir);

			CalculatedRadii.Add(GetWaterfallSettings()->SplashScale * FMath::Lerp(GetWaterfallSettings()->SplashFrontRadius, GetWaterfallSettings()->SplashBackRadius, (float)j / FMath::Max(GetWaterfallSettings()->SplashRadiusSubdivisions - 1, 1)));

			FVector LocalDir = LocalDirections.Last().RotateAngleAxis(RotationAngleDegrees, FVector(0.f, 0.f, 1.f).GetSafeNormal());
			LocalDirections.Add(LocalDir);

			LocalDistances.Add(MarchedDistance - (TotalDistance * 0.5f));
		}

		//Construct the back part of the mesh doing the same thing we did with the front part of the mesh
		for (int j = 0; j < WorkingPositions.Num(); j++)
		{
			int32 WorkingIndex = WorkingPositions.Num() - 1 - j;
			CalculatedPositions.Add(WorkingPositions[WorkingIndex]); //Add the position
			CalculatedNormals.Add(WorkingNormals[WorkingIndex]); //Add the normal
			LocalDirections.Add(FVector(0.f, 1.f, 0.f)); //Add the direction

			if (WorkingIndex != WorkingPositions.Num() - 1) MarchedDistance -= FVector::Dist(WorkingPositions[WorkingIndex], WorkingPositions[WorkingIndex + 1]);
			LocalDistances.Add(MarchedDistance - (TotalDistance * 0.5f));

			CalculatedRadii.Add(GetWaterfallSettings()->SplashBackRadius * GetWaterfallSettings()->SplashScale);

			FVector Dir = FVector::ZeroVector;
			if (WorkingIndex > 0 && WorkingIndex < WorkingPositions.Num() - 1) //First and last are slightly different
			{
				Dir = USH_WaterfallTool2Statics::SlerpNormals(
					UKismetMathLibrary::GetDirectionUnitVector(WorkingPositions[WorkingIndex], WorkingPositions[WorkingIndex - 1]).Cross(FVector(0.f, 0.f, 1.f)),
					UKismetMathLibrary::GetDirectionUnitVector(WorkingPositions[WorkingIndex + 1], WorkingPositions[WorkingIndex]).Cross(FVector(0.f, 0.f, 1.f)),
					0.5f);
			}
			else if (WorkingIndex > 0)
			{
				Dir = UKismetMathLibrary::GetDirectionUnitVector(WorkingPositions[WorkingIndex], WorkingPositions[WorkingIndex - 1]).Cross(FVector(0.f, 0.f, 1.f));
			}
			else
			{
				Dir = UKismetMathLibrary::GetDirectionUnitVector(WorkingPositions[WorkingIndex + 1], WorkingPositions[WorkingIndex]).Cross(FVector(0.f, 0.f, 1.f));
			}

			CalculatedDirections.Add(Dir);
		}

		//Rotate around the first point to connect back to the start of the path
		for (int j = 0; j <= GetWaterfallSettings()->SplashRadiusSubdivisions; j++)
		{
			CalculatedPositions.Add(WorkingPositions[0]);
			FVector LastNormal = CalculatedNormals.Last();
			CalculatedNormals.Add(LastNormal);

			float RotationAngleDegrees = 180.f / (FMath::Max(GetWaterfallSettings()->SplashRadiusSubdivisions, 1) + 1);

			FVector Dir = CalculatedDirections.Last().RotateAngleAxis(RotationAngleDegrees, FVector(0.f, 0.f, 1.f));
			CalculatedDirections.Add(Dir);

			CalculatedRadii.Add(GetWaterfallSettings()->SplashScale * FMath::Lerp(GetWaterfallSettings()->SplashBackRadius, GetWaterfallSettings()->SplashFrontRadius, FMath::Clamp((float)j / FMath::Max(GetWaterfallSettings()->SplashRadiusSubdivisions, 1), 0.f, 1.f)));

			FVector LocalDir = LocalDirections.Last().RotateAngleAxis(RotationAngleDegrees, FVector(0.f, 0.f, 1.f));
			LocalDirections.Add(LocalDir);

			LocalDistances.Add(0 - (TotalDistance * 0.5f));
		}

		//Apply the overall scale
		for (int j = 0; j < CalculatedPositions.Num(); j++)
		{
			CalculatedPositions[j] = (CalculatedPositions[j] - ScaleOrigin) * GetWaterfallSettings()->SplashScale + ScaleOrigin;
		}
	}

	//4. Extrude Splash Path
	{
		int32 VertexOffset = Positions.Num();

		float OuterPerimeter = 0.f;
		for (int j = 1; j < CalculatedPositions.Num(); j++) //Skip the first index
		{
			OuterPerimeter += FVector::Dist(CalculatedPositions[j] + (CalculatedDirections[j] * CalculatedRadii[j]), CalculatedPositions[j - 1] + (CalculatedDirections[j - 1] * CalculatedRadii[j - 1]));
		}

		float OuterMarchedDistance = 0.f;
		for (int j = 0; j < CalculatedPositions.Num(); j++)
		{
			float OuterDistance = (j <= 0) ? 0.f : FVector::Dist(CalculatedPositions[j] + (CalculatedDirections[j] * CalculatedRadii[j]), CalculatedPositions[j - 1] + (CalculatedDirections[j - 1] * CalculatedRadii[j - 1]));
			OuterMarchedDistance += OuterDistance;

			//Using the subdivisions, create the vertices and associated data
			int32 LastIndex = FMath::Max(GetWaterfallSettings()->SplashSegments + 1, 1);
			for (int k = 0; k <= LastIndex; k++)
			{
				float DistFallOffCalc = FMath::Pow((float)k / LastIndex, GetWaterfallSettings()->SplashDistanceFalloff);

				Positions.Add(CalculatedPositions[j] + (CalculatedDirections[j] * CalculatedRadii[j] * DistFallOffCalc));
				Normals.Add(CalculatedNormals[j]);

				UVs0.Add(FVector2D(OuterMarchedDistance / OuterPerimeter, 1.f - FMath::Pow(DistFallOffCalc, GetWaterfallSettings()->SplashUVFalloff)));

				FVector UV1 = FVector(LocalDistances[j], 0.f, 0.f);
				UV1 += (LocalDirections[j] * CalculatedRadii[j] * DistFallOffCalc);
				UVs1.Add(FVector2D(UV1.X, UV1.Y));

				MeshBuffers_CalculateUV2(Values, 1, 1);
				MeshBuffers_CalculateUV3(Values, CurrentPath, 1);

				MeshBuffers_CalculateColour(Values, CalculatedDirections[j], 1);

				//Don't process the last line of vertices because there's nothing more to connect them to!
				if (j < CalculatedPositions.Num() - 1 && k < LastIndex)
				{
					/*Build two triangles to make a quad*/
					int32 A = k + ((j + 1) * (LastIndex + 1)) + VertexOffset;
					int32 C = (j * (LastIndex + 1)) + k + VertexOffset;
					int32 B = C + 1;
					int32 D = A + 1;

					Triangles.Add(FIntVector(A, B, C));
					Triangles.Add(FIntVector(A, D, B));
				}
			}
		}
	}
}

FVector FSH_WaterfallBuilder_Mesh::GetProjectedPosition(FVector Position, FVector Direction, float Distance, float Offset, float Alpha, bool bFlattened, bool bProject, ESH_MeshProjectionType ProjectionType)
{
	if (Distance > 0.f && Alpha > 0.f)
	{
		FVector ProjectedPosition = FVector::ZeroVector;
		FVector FlattenedMultiplier = (bFlattened) ? FVector(1.f, 1.f, 0.f) : FVector::OneVector;

		//Trace away from the object
		auto TracePositive = [=, this]() -> FHitResult
		{
			FHitResult OutHit;
			UKismetSystemLibrary::LineTraceSingle(
				GetParentWaterfall(),
				Position + (Direction * (Distance * FlattenedMultiplier)),
				Position - (FlattenedMultiplier * Offset),
				GetWaterfallSettings()->GetSimulationTraceChannel(),
				true,
				{},
				EDrawDebugTrace::None,
				OutHit,
				true
			);

			return OutHit;
		};

		//Trace towards the object
		auto TraceNegative = [=, this]() -> FHitResult
		{
			FHitResult OutHit;
			UKismetSystemLibrary::LineTraceSingle(
				GetParentWaterfall(),
				ProjectedPosition,
				ProjectedPosition - (Direction * Distance * FlattenedMultiplier),
				GetWaterfallSettings()->GetSimulationTraceChannel(),
				true,
				{},
				EDrawDebugTrace::None,
				OutHit,
				true
			);

			return OutHit;
		};

		//Nudge or push
		auto ProcessProjectedPosition = [&](FHitResult Hit)
		{
			if (Hit.bBlockingHit)
			{
				ProjectedPosition = FMath::Lerp(Position, Hit.ImpactPoint + (Hit.ImpactNormal * Offset * FlattenedMultiplier), Alpha);
			}
			else if (bProject)
			{
				ProjectedPosition = FMath::Lerp(Position, Hit.TraceEnd, Alpha);
			}
			else
			{
				ProjectedPosition = Position;
			}
		};

		if (ProjectionType == ESH_MeshProjectionType::W2_MP_Positive || ProjectionType == ESH_MeshProjectionType::W2_MP_Both)
		{
			ProcessProjectedPosition(TracePositive());
		}

		if (ProjectionType == ESH_MeshProjectionType::W2_MP_Negative || ProjectionType == ESH_MeshProjectionType::W2_MP_Both)
		{
			ProcessProjectedPosition(TraceNegative());
		}

		return ProjectedPosition;
	}

	return Position;
}

void FSH_WaterfallBuilder_Mesh::ProcessingFinished()
{
	FSH_WaterfallBuilder::ProcessingFinished();

	bSingularCachedPaths = false;

	//If there are more meshes to be generated in the queue, start the next mesh generation
	if (QueuedMeshesToBeGenerated.Num() > 0)
	{
		StartMeshGeneration(OnEndProcessing);
	}
	else //Reenable the slate and finish completely
	{
		OnEndProcessing.ExecuteIfBound();
	}
}
#endif