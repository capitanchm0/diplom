// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "SH_WaterfallPathComponent.generated.h"

/* For path generation - the various states that a point can be when simulating. */
UENUM()
enum class ESH_SimulatedPointState : uint8
{
	Start,
	Airborn,
	Hit,
	Slide,
	Stalled,
	Killed,
	Blocked,
	Died,
};

/* All of the generated data for a single simulated point. */
USTRUCT()
struct FSH_SimulatedPoint
{
	GENERATED_BODY()

	FSH_SimulatedPoint() {}

#if WITH_EDITORONLY_DATA
	UPROPERTY()
		FVector Velocity = FVector::ZeroVector;
	UPROPERTY()
		ESH_SimulatedPointState State = ESH_SimulatedPointState::Start;
	UPROPERTY()
		float Distance = 0.f;
	UPROPERTY()
		FVector HitNormal = FVector::ZeroVector;
	UPROPERTY()
		float HitForce = 0.f;
	UPROPERTY()
		float Age = 0.f;
	UPROPERTY()
		float ObstacleDeviation = 0.f;
	UPROPERTY()
		float FlowDeviation = 0.f;
	UPROPERTY()
		bool bHasFxAttached = false;
	UPROPERTY()
		int32 PointIndex = INDEX_NONE;

	FSH_SimulatedPoint(FVector _Velocity, ESH_SimulatedPointState _State, float _Distance = 0.f, FVector _HitNormal = FVector::ZeroVector, float _HitForce = 0.f, float _Age = 0.f, float _ObstacleDeviation = 0.f, float _FlowDeviation = 0.f)
	{
		Velocity = _Velocity;
		State = _State;
		Distance = _Distance;
		HitNormal = _HitNormal;
		HitForce = _HitForce;
		Age = _Age;
		ObstacleDeviation = _ObstacleDeviation;
		FlowDeviation = _FlowDeviation;
	}
#endif //WITH_EDITORONLY_DATA
};

#if WITH_EDITORONLY_DATA
/* Used for the flow deviation calculations. */
struct FSH_SearchTreeNode
{
	FBox Bounds = FBox();
	bool bIsLeaf = false;

	//The indexes of the simulated points that this node covers
	int32 FirstPointIndex = -1;
	int32 LastPointIndex = -1;
	
	//The index of this node in the tree
	int32 NodeIndex = -1;

	TArray<FSH_SearchTreeNode> ChildNodes = {};

	FSH_SearchTreeNode() {}
	FSH_SearchTreeNode(int32 _NodeIndex, int32 _FirstPointIndex, int32 _LastPointIndex)
	{
		NodeIndex = _NodeIndex;
		FirstPointIndex = _FirstPointIndex;
		LastPointIndex = _LastPointIndex;
	}

	int32 FindNearestPoint(FVector Position, float& NearestDistance, class USH_WaterfallPathComponent* WaterfallComp);

	bool IsValid()
	{
		return NodeIndex >= 0;
	}
};

/* Used for the flow deviation calculations. */
struct FSH_PointDistanceNode
{
	FSH_SearchTreeNode Node = FSH_SearchTreeNode();
	float Distance = TNumericLimits<float>::Max();

	FSH_PointDistanceNode() {}
	FSH_PointDistanceNode(FSH_SearchTreeNode _Node, float _Distance)
	{
		Node = _Node;
		Distance = _Distance;
	}

	bool operator<(const FSH_PointDistanceNode& b) const
	{
		return Distance < b.Distance;
	}
};
#endif //WITH_EDITORONLY_DATA

/* Represents a point between simulated points. */
USTRUCT()
struct FSH_WaterfallSample
{
	GENERATED_BODY()

	FSH_WaterfallSample() {}

#if WITH_EDITORONLY_DATA
	int32 Index = 0;
	float Alpha = 0.f;

	FSH_WaterfallSample(int32 _Index, float _Alpha)
	{
		Index = _Index;
		Alpha = _Alpha;
	}
#endif //WITH_EDITORONLY_DATA
};

#if WITH_EDITORONLY_DATA
class ASH_Waterfall2;
class USH_WaterfallSettingsComponent;
#endif //WITH_EDITORONLY_DATA

/* A Spline Component that represents a single simulated path for the waterfall.
* Note: This component is currently editor-only and not available at runtime. */
UCLASS()
class SHADERSOURCE_WATERFALL2RUNTIME_API USH_WaterfallPathComponent : public USplineComponent
{
	GENERATED_BODY()

	friend class USH_WaterfallGenerationComponent;
	friend class FSH_PathCompVisualiser;
	friend class SSH_FxPathDisplay;
	friend struct FSH_PositionPathPair;
	friend struct FSH_WaterfallBuilder_Path;
	friend struct FSH_WaterfallBuilder_Mesh;

public:
	USH_WaterfallPathComponent();

	//For sorting
	bool operator<(const USH_WaterfallPathComponent& b) const
	{
		return PathIndex < b.PathIndex;
	}

	//The index of the path along the Top Spline from left to right
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waterfall")
		int32 PathIndex = INDEX_NONE;

#if WITH_EDITORONLY_DATA
	//Note: This can return nullptr so it should be checked
	ASH_Waterfall2* GetParentWaterfall();
	//Note: This can return nullptr so it should be checked
	USH_WaterfallSettingsComponent* GetWaterfallSettings();

private:
	ASH_Waterfall2* ParentWaterfall = nullptr;
	USH_WaterfallSettingsComponent* SettingsComponent = nullptr;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waterfall")
		float GeneratedPathSeed = 0.f;

	virtual bool HasBeenSimulated() { return bIsSimulated; }

	UPROPERTY()
		float SplineDistance = 0.f;
	UPROPERTY()
		float NormalisedSplineDistance = 0.f;
	UPROPERTY()
		float UVSeed = 0.f;

	virtual bool IsInBounds(FVector Position, float Margin);

	FVector GetSimulatedPointPosition(int32 Index);
	int32 GetNumSimulatedPoints() { return SimulatedPoints.Num(); }
	bool DoesSimulatedPointHaveFX(int32 Index);
	void SetSimulatedPointHasFX(int32 Index, bool bHasFX);

	virtual void ResamplePath(int32 NumPoints);

protected:
	virtual void AddNewSimulatedPoint(FVector Position, FSH_SimulatedPoint NewPoint);
	virtual void UpdateSimulatedPointPosition(int32 Index, FVector NewPosition);

	virtual void CalculateIntialSimulationVariables();
	virtual FVector GetProjectedDirectionFromHitNormal(FSH_SimulatedPoint CurrentPoint, FVector ImpactNormal, float MinSpeed = 1.f);
	virtual bool SimulatePath(float DeltaTime, int32& Iteration, TArray<USH_WaterfallPathComponent*> PathsForDeviation);

	virtual void ResetSimulationVariables();

	virtual void SetSampleLength(float SuggestedLength, int32 MinSamples, int32 MaxSamples);
	virtual void SetSampleIncludeHits(bool bIncludeHits, float Threshold);

	virtual float GetSimulationDistance();
	virtual float GetSimulationBoundsHeight();

	virtual void InvalidateSampleCache();
	virtual void InvalidatePointsCache();
	virtual bool CachePath();

	virtual bool IsPathCacheConsistent();
	
	//Resample the cache to add or remove points to make it consistent with other paths
	virtual bool ResampleCache(int32 NumPoints);

private:
	UPROPERTY()
		bool bIsSimulated = false;
	UPROPERTY()
		TArray<FSH_SimulatedPoint> SimulatedPoints = {};
	UPROPERTY()
		FBox SimulationBounds = FBox();
	UPROPERTY()
		float SimulationMaxHitForce = 0.f;
	UPROPERTY()
		float SimulationMaxSpeed = 0.f;

public:
	FSH_SimulatedPoint GetLastSimulatedPoint() { return (SimulatedPoints.Num() > 0) ? SimulatedPoints[SimulatedPoints.Num() - 1] : FSH_SimulatedPoint(); }

protected:
	bool BuildSearchTree(int32 Precision = 4);
	FSH_SearchTreeNode BuildSearchNode(int32 FirstIndex, int32 LastIndex, int32 Layer, int32 Precision, int32& OutMaxIndex, FSH_SearchTreeNode* ParentNode = nullptr);
	bool FindNearestPoint(FVector Position, FSH_SimulatedPoint& OutPoint);

public:
	int32 GetLastSelectedSplinePoint() { return LastSelectedSplinePoint; }

protected:
	int32 LastSelectedSplinePoint = INDEX_NONE;

	//Used for flow alignment and flow repulsion calculations to compare with points in other paths
	FSH_SearchTreeNode SearchTree = FSH_SearchTreeNode();

	int32 CalculatedSampleCount = -1;
	float CalculatedSampleLength = -1.f;

	TArray<FSH_WaterfallSample> SampledCache = {};
	int32 SampledCount = 0;
	bool bSampled = false;

	TArray<FVector> PositionsCache = {};
	int32 SmoothedPositionIteration = 0;
	bool bPositionsCached = false;

	TArray<FVector> VelocitiesCache = {};
	bool bVelocitiesCached = false;

	TArray<FVector> NormalsCache = {};
	int32 SmoothedNormalIteration = 0;
	bool bNormalsCached = false;

	TArray<FVector> TangentsCache = {};
	int32 SmoothedTangentIteration = 0;
	bool bTangentsCached = false;

	TArray<float> DistanceCache = {};
	TArray<float> NormalisedDistanceCache = {};
	bool bDistanceCached = false;

	TArray<float> FlowCache = {};
	bool bFlowCached = false;

	TArray<float> TurbulenceCache = {};
	bool bTurbulenceCached = false;

	TArray<float> HitsCache = {};
	bool bHitsCached = false;

	bool bSampleIncludeHits = false;
	float SampleThreshold = 0.f;

	TArray<float> RandomValuesPool = {};

protected:
	virtual bool SamplePath();
	virtual bool FindSampleByDistance(float Distance, int32 StartIndex, FSH_WaterfallSample& OutSample);
	virtual TArray<int32> FindHitsBetweenPoints(float HitForce, FSH_WaterfallSample Start, FSH_WaterfallSample End);
	virtual bool GetInterpolatedSimulatedPoint(FSH_WaterfallSample Sample, FSH_SimulatedPoint& OutPoint);
	virtual FSH_SimulatedPoint InterpolateSimulatedPoint(int32 IndexA, int32 IndexB, float Alpha, FVector& InterpolatedPosition);

	virtual FVector GetPointPosition(int32 Index);
	virtual void SmoothPositions(FVector Alpha);
	virtual bool PathTerminatesInWater();
	virtual float GetSimulationWaterLevel();
	virtual FVector GetPointNormal(int32 Index);
	virtual void SmoothNormals(float Alpha);
	virtual FVector GetPointDirection(int32 Index);
	virtual FVector GetPointTangent(int32 Index);
	virtual void SmoothTangents(float Alpha);
	virtual FVector GetPointVelocity(int32 Index);
	virtual void GetPointDistance(int32 Index, float& Distance, float& DistanceNormalised);
	virtual float GetPointFlow(int32 Index, float Min = 1.f);
	virtual float GetPointTurbulence(int32 Index);
	virtual float GetPointInterpolatedTurbulence(FSH_SimulatedPoint A, FSH_SimulatedPoint B, float Alpha);
	virtual void PropagateTurbulence();
	virtual float GetPointHit(int32 Index);

#endif //WITH_EDITORONLY_DATA
};
