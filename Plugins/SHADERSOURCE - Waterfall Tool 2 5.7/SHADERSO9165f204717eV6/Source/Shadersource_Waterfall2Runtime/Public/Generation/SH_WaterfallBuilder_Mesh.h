// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Generation/SH_WaterfallBuilder.h"
#include "EditorComponents/SH_WaterfallGenerationEnums.h"

class USH_WaterfallSettingsComponent;

#if WITH_EDITOR
/* Custom paths made out of points from various other paths
* (used for Singular Mesh generation for sorting points). */
struct FSH_RemappedWaterfallPath
{
	TArray<struct FSH_PositionPathPair> Points = {};
	int32 RemappedArrayIndex = INDEX_NONE;

	FSH_RemappedWaterfallPath() {}
	FSH_RemappedWaterfallPath(int32 _RemappedArrayIndex)
	{
		RemappedArrayIndex = _RemappedArrayIndex;
	}

	//Helper to add a new point
	void Add(FSH_PositionPathPair NewPoint);
	//Helper for Points.Num()
	int32 Num() const;

	FVector GetPosition(int32 Index) const;
	FVector GetNormal(int32 Index) const;
	FVector GetTangent(int32 Index) const;
	float GetDistance(int32 Index) const;
	float GetNormalisedDistance(int32 Index) const;
	float GetTurbulence(int32 Index) const;
	float GetFlow(int32 Index) const;
	FVector GetVelocity(int32 Index) const;
};

/* A key pair holding an Original Path and Position Index pointer
* (used for Singular Mesh generation for sorting points). */
struct FSH_PositionPathPair
{
	USH_WaterfallPathComponent* OriginalPath = nullptr;
	int32 PositionIndex = INDEX_NONE;

	FSH_PositionPathPair() {}
	FSH_PositionPathPair(int32 _PositionIndex, USH_WaterfallPathComponent* _OriginalPath)
	{
		PositionIndex = _PositionIndex;
		OriginalPath = _OriginalPath;
	}

	FVector GetPosition() const;
	FVector GetNormal() const;
	FVector GetTangent() const;
	float GetDistance() const;
	float GetNormalisedDistance() const;
	float GetTurbulence() const;
	float GetFlow() const;
	FVector GetVelocity() const;

	static bool LineSegmentIntersection(FVector2D line1point1, FVector2D line1point2, FVector2D line2point1, FVector2D line2point2);
};

/* A key pair holding an Original Path, Position Index pointer, and a new remapped path
* (used for Singular Mesh generation for sorting points). */
struct FSH_PositionPathPairWithPath : public FSH_PositionPathPair
{
	FSH_RemappedWaterfallPath RemappedPath = FSH_RemappedWaterfallPath();

	FSH_PositionPathPairWithPath(FSH_PositionPathPair _OtherPosition, FSH_RemappedWaterfallPath _RemappedPath)
	{
		RemappedPath = _RemappedPath;
		PositionIndex = _OtherPosition.PositionIndex;
		OriginalPath = _OtherPosition.OriginalPath;
	}

	bool operator<(const FSH_PositionPathPairWithPath b) const;
};
#endif

/* Builder that generates dynamic mesh data from path data. */
struct SHADERSOURCE_WATERFALL2RUNTIME_API FSH_WaterfallBuilder_Mesh : public FSH_WaterfallBuilder
{
	friend class ASH_Waterfall2;

public:
	FSH_WaterfallBuilder_Mesh() {}
	FSH_WaterfallBuilder_Mesh(ASH_Waterfall2* _ParentWaterfall)
		: FSH_WaterfallBuilder(_ParentWaterfall)
	{

	}

#if WITH_EDITOR
	//Mesh Generation
public:
	virtual void StartMeshGeneration(FSimpleDelegate OnProcessingFinished = FSimpleDelegate());
	virtual void CancelMeshGeneration(bool bShowEditorNotification = true);

	TArray<ESH_MeshGenerationType> QueuedMeshesToBeGenerated = {};

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void TickMeshGeneration_SingularRemesh(float DeltaTime);
	virtual void TickMeshGeneration_SingularNoRemesh(float DeltaTime);
	virtual void TickMeshGeneration_OnePath(float DeltaTime);
	virtual void EndMeshGeneration();

	//Convert path components to remapped path structs
	TArray<FSH_RemappedWaterfallPath> Singular_CovertToRemappedPaths();
	TArray<FSH_RemappedWaterfallPath> Singular_RemapPoints(bool bPrintPaths);
	void ApplySamplingSettings();
	void ClearMeshBuffers();

private:
	TArray<FSH_RemappedWaterfallPath> SingularRemappedPaths = {};
	bool bSingularCachedPaths = false;

public:
	void Debug_ShowVertices(float Duration, FIntVector2 DebugPointsShowOnlyThesePoints);
	void Debug_ShowTriangles(float Duration);
	void Debug_ShowDirections(float Duration);
	void Debug_PrintRemappedPaths();

protected:
	void BuildMeshBuffers_SingularRemesh(FSH_RemappedWaterfallPath CurrentPath, FSH_RemappedWaterfallPath NextPath, float NextPathInterp, float PathAlpha, int32 PathIndex);
	void BuildMeshBuffers_SingularNoRemesh(FSH_RemappedWaterfallPath CurrentPath, FSH_RemappedWaterfallPath NextPath);

	struct FOS_PathValues
	{
		bool bFlattenLastIndex = false;

		FVector Position = FVector::ZeroVector;
		FVector Normal = FVector::ZeroVector;
		FVector Tangent = FVector::ZeroVector;

		float Distance = 0.f;
		float NormalisedDistance = 0.f;

		float Turbulence = 0.f;
		float Flow = 0.f;

		FVector Velocity = FVector::ZeroVector;
		float Speed = 0.f;

		float Width = 0.f;

		int32 Index = -1;

		FOS_PathValues() {}
		FOS_PathValues(USH_WaterfallPathComponent* CurrentPath, int32 i, USH_WaterfallSettingsComponent* WaterfallSettings);
		FOS_PathValues(FSH_RemappedWaterfallPath CurrentPath, int32 i);
		FOS_PathValues(FSH_RemappedWaterfallPath CurrentPath, FSH_RemappedWaterfallPath NextPath, int32 i, float NextPathInterp, USH_WaterfallSettingsComponent* WaterfallSettings);
	};

	void MeshBuffers_CalculatePositions(TArray<FVector>& PositionsArray, FOS_PathValues& Values, int32 Subdivisions, FVector OffsetVector);
	void MeshBuffers_CalculateNormals(TArray<FVector>& NormalsArray, FOS_PathValues& Values, float PathAlpha, USH_WaterfallPathComponent* CurrentOriginalPath);
	void MeshBuffers_CalculateNormals(TArray<FVector>& NormalsArray, const FOS_PathValues& Values, int32 Subdivisions);
	void MeshBuffers_CalculateColour(const FOS_PathValues& Values, FVector Direction, int32 NumToAdd);
	void MeshBuffers_CalculateUV2(const FOS_PathValues& Values, float YScale, int32 NumToAdd);
	void MeshBuffers_CalculateUV3(const FOS_PathValues& Values, USH_WaterfallPathComponent* CurrentPath, int32 NumToAdd);

	void BuildMeshBuffers_OnePath(USH_WaterfallPathComponent* CurrentPath, int32 Subdivisions);
	void BuildMeshBuffers_Splash(USH_WaterfallPathComponent* CurrentPath);

	FVector GetProjectedPosition(FVector Position, FVector Direction, float Distance, float Offset, float Alpha, bool bFlattened, bool bProject, ESH_MeshProjectionType ProjectionType);

public:
	virtual void ProcessingFinished() override;

private:
	TArray<FVector> Positions = {};
	TArray<FVector> Normals = {};

	TArray<FLinearColor> Colours = {};
	//0:1 UVs
	TArray<FVector2D> UVs0 = {};
	//In centimetres
	TArray<FVector2D> UVs1 = {};
	//Speed/flow
	TArray<FVector2D> UVs2 = {};
	//Seed/local position
	TArray<FVector2D> UVs3 = {};
	TArray<FIntVector> Triangles = {};
	TArray<FLinearColor> FxData1 = {};
	TArray<FLinearColor> FxData2 = {};

	TMap<int32, int32> VerticesIndexes = {};
#endif
};
