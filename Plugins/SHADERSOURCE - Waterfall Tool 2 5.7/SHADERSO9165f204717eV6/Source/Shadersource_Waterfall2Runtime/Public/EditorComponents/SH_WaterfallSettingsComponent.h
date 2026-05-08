// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EditorComponents/SH_WaterfallGenerationEnums.h"
#include "SH_WaterfallSettingsComponent.generated.h"

#if WITH_EDITORONLY_DATA
class ASH_Waterfall2;
class UNiagaraSystem;
class USoundBase;
#endif //WITH_EDITORONLY_DATA

/* A component that contains the Editor Only settings for the Waterfall Ed Mode.
* None of these settings are needed/available at runtime.
* It's also not a BlueprintSpawnable Component because it shouldn't be able to be added to Blueprints */
UCLASS(AutoCollapseCategories = ("Simulation", "Speed", "Direction", "Physics", "Avoidance", "Flow"))
class SHADERSOURCE_WATERFALL2RUNTIME_API USH_WaterfallSettingsComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class FSH_Details_WaterfallEdit;

public:
	USH_WaterfallSettingsComponent();

#if WITH_EDITORONLY_DATA
	//Note: This can return nullptr so it should be checked
	ASH_Waterfall2* GetParentWaterfall();

	/* Random seed for the generation of the waterfall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random")
		int32 Seed = 0;
	/* Returns the random stream that uses the specified seed. */
	FRandomStream Random() { return SeedStream; }
	/* Randomise the seed with a new number. */
	FReply ButtonClicked_RandomiseSeed();

	/* Return the widget associated with the current selected tab. */
	TSharedPtr<SWidget> GetTabActionWidget();

protected:
	/* Generate the paths for the waterfall. */
	FReply ButtonClicked_GeneratePaths();
	/* Delete all the waterfall paths. */
	FReply ButtonClicked_DeletePaths();
	/* Cancel the generation of the waterfall paths. */
	void CancelGeneration_Paths();

	/* Generate the dynamic mesh waterfall data. */
	FReply ButtonClicked_GenerateMeshes();
	/* Cancel the generation of the dynamic mesh waterfall data. */
	void CancelGeneration_Meshes();
	/* Remove all the generated dynamic mesh data. */
	FReply ButtonClicked_ClearDynamicMeshes();
	/* Remove the generated dynamic mesh for a specific mesh type. */
	FReply ButtonClicked_ClearSpecificMesh();

	/* Convert the dynamic mesh data to a static mesh asset. */
	FReply ButtonClicked_BakeToStaticMesh();
	
	/* Generate the Fx. */
	FReply ButtonClicked_GenerateFx();

	/* Show the generated dynamic mesh vertices.
	* (Note: This data does not persist when the editor is closed or a new level is loaded.) */
	FReply ButtonClicked_Debug_ShowVertices();
	/* Show the generated dynamic mesh triangles.
	* (Note: This data does not persist when the editor is closed or a new level is loaded.) */
	FReply ButtonClicked_Debug_ShowTriangles();
	/* Show the generated path directions for each point.
	* (Note: This data does not persist when the editor is closed or a new level is loaded.) */
	FReply ButtonClicked_Debug_ShowDirections();
	/* Print the remapped points for single mesh generation to the log.
	* (Note: This data does not persist when the editor is closed or a new level is loaded.) */
	FReply ButtonClicked_Debug_PrintRemappedPoints();

	/* Set the Ed Mode widget enabled or disabled. */
	void SetWidgetEnabled(bool bEnabled, FSimpleDelegate OptionalCancelDelegate = FSimpleDelegate());

	/* Set the entire Ed Mode widget enabled. */
	void SetToolkitWidgetEnabled() { SetWidgetEnabled(true); }
	/* Set the entire Ed Mode widget disabled. */
	void SetToolkitWidgetDisabled(FSimpleDelegate OptionalCancelDelegate = FSimpleDelegate()) { SetWidgetEnabled(false, OptionalCancelDelegate); }

private:
	/* A stored pointer to the parent waterfall actor. */
	ASH_Waterfall2* ParentWaterfall = nullptr;
	/* The random stream made from the seed. */
	FRandomStream SeedStream = FRandomStream();
	/* Make sure the stream is correctly initialised from the current seed. */
	void VerifySeedStream();

	//Debug
public:
	/* How long to show the debug lines/points for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		float DebugPointsDuration = 15.f;

#endif
	/* Limit the points shown when using the debug buttons. */
	UPROPERTY(EditAnywhere, Category = "Debug")
		FIntVector2 DebugPointsShowOnlyThesePoints = FIntVector2(-1);

	UFUNCTION(BlueprintPure, Category = "Debug")
		int32 GetDebugPointsShowOnlyThesePointsX() { return DebugPointsShowOnlyThesePoints.X; }
	UFUNCTION(BlueprintPure, Category = "Debug")
		int32 GetDebugPointsShowOnlyThesePointsY() { return DebugPointsShowOnlyThesePoints.Y; }

	UFUNCTION(BlueprintCallable, Category = "Debug")
		void SetDebugPointsShowOnlyThesePoints(const int32& X, const int32& Y)
	{
		DebugPointsShowOnlyThesePoints = FIntVector2(X, Y);
	}
#if WITH_EDITORONLY_DATA

	//Generate Paths
protected:
	/* The number of paths to generate along the spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", meta = (ClampMin = "1"))
		int32 NumPaths = 10;

	/* Number of iterations per frame for the simulation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
		int32 Substeps = 50;

public:
	/* Set the number of spline points on each waterfall path to a specific number. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn")
		bool bResampleSpline = true;
	/* If on, after the path is generated, it will resample the path to exactly this many spline points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", meta = (ClampMin = "2", EditCondition = "bResampleSpline"))
		int32 NumSplinePoints = 35;

protected:
	/* The closer to 0, the more points are going to spawn, the further away from 0, the less points are going to spawn.
	* This is taken into account BEFORE any resampling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", AdvancedDisplay, meta = (DisplayName = "Point Grouping", ClampMin = "0", ClampMax = "0.5"))
		float SimulationDeltaTime = 0.016667f;

	/* If only one path is required to spawn, this will be the 0-1 offset on the spline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", meta = (ClampMin = "0", ClampMax = "1"))
		float SinglePathPosition = 0.5f;

	/*If more than one path is required to spawn, this will confine the spawning to a range from the end
	* Eg, If the spline distance is 100 and the PathSpawnRange is set to (0.2, 0.8), then the
	* paths will only spawn between distances 20-80 and none will spawn on the distance < 20 or > 80
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", meta = (ClampMin = "0", ClampMax = "1"))
		FVector2D PathSpawnRange = FVector2D(0.f, 1.f);

	/* Whether to add directional jitter to the start of the waterfall paths or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", meta = (InlineEditConditionToggle))
		bool bAddSpawnJitter = false;
	/* Affects the initial direction of the path by adding a randomised jitter to the yaw in this range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", meta = (ClampMin = "-180", ClampMax = "180", EditCondition = "bAddSpawnJitter"))
		FVector2D SpawnJitterRange = FVector2D(-10.f, 10.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", AdvancedDisplay)
		float SpawnTraceDistance = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", AdvancedDisplay)
		float SpawnTraceOffset = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InitialSpawn", AdvancedDisplay, meta = (ClampMin = "0", ClampMax = "1"))
		float SpawnTraceAlpha = 1.f;

	/* Which channel should the simulation run traces on (1 is visibility). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
		TEnumAsByte<ETraceTypeQuery> SimulationTraceChannel = ETraceTypeQuery::TraceTypeQuery1;

public:
	/* Additional actors that terminate path simulations other than the kill plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kill")
		TArray<AActor*> AdditionalKillActors = {};
	/* Additional scale offset for the kill plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kill")
		FVector KillPlaneScaleOffset = FVector(1.f);

public:
	/* Simulation speed of the particle for the path generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
		float Speed = 400.f;
	/* Maximum simulation speed of the particle for the path generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
		float MaxSpeed = 2000.f;
	/* Additional random speed of the particle for the path generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
		float SpeedRandom = 0.f;
	/* If the particle moves slower than this speed in the simulation,
	* the path will terminate even if it's not touching a kill actor/kill plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
		float KillSpeed = 25.f;
	/* Changes the velocity to work in the opposite direction on the spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Direction")
		bool bFlipSplineDirection = false;

	/* Particle simulation gravity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
		float Gravity = -980.f;
	/* Particle simulation drag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
		float Drag = 0.1f;
	/* Particle simulation friction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
		float Friction = 2.f;
	/* Particle simulation bounce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics", meta = (ClampMin = "0", ClampMax = "1"))
		float Bounce = 0.f;
	/* Particle simulation lifetime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
		float Lifetime = 10.f;

	/* Particle simulation obstacle avoidance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
		float ObstacleAvoidanceWeight = 20.f;
	/* Particle simulation avoidance distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
		float ObstacleAvoidanceDistance = 1.f;
	/* Particle simulation avoidance falloff. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
		float ObstacleAvoidanceFalloff = 1.f;

	/* Particle simulation obstacle repulsion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
		float ObstacleRepulsionWeight = 2.f;
	/* Particle simulation obstacle repulsion distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
		float ObstacleRepulsionDistance = 0.1f;
	/* Particle simulation obstacle repulsion falloff. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
		float ObstacleRepulsionFalloff = 1.f;
	/* Actors to ignore for collision detection and physics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", meta = (DisplayName = "Ignore Actors"))
		TArray<AActor*> IgnoreActorsForAvoidance = {};

	/* Particle simulation flow repulsion (how much each path pushes the paths around it away from it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
		float FlowRepulsionWeight = 1000.f;
	/* Particle simulation flow repulsion distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
		float FlowRepulsionDistance = 75.f;
	/* Particle simulation flow repulsion falloff. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
		float FlowRepulsionFalloff = 2.f;

	/* Particle simulation flow alignment (how much each path attracts the paths around it towards it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
		float FlowAlignmentWeight = 2.f;
	/* Particle simulation flow alignment distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
		float FlowAlignmentDistance = 150.f;
	/* Particle simulation flow alignment falloff. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
		float FlowAlignmentFalloff = 2.f;

public:
	/* Get the number of paths to generate. */
	virtual int32 GetNumPaths();
	/* Get the number of substeps per tick. */
	virtual int32 GetSubsteps();
	/* Get the simulation delta time variable. */
	virtual float GetSimulationDeltaTime();
	/* Get the single path position. */
	virtual float GetSinglePathPosition() { return SinglePathPosition; }
	/* Get the path spawn range. */
	virtual FVector2D GetPathSpawnRange() { return PathSpawnRange; }

	/* Get the spawn jitter. */
	virtual bool GetAddSpawnJitter();
	/* Get the spawn jitter min. */
	virtual float GetSpawnJitterMin() { return SpawnJitterRange.X; }
	/* Get the spawn jitter max. */
	virtual float GetSpawnJitterMax() { return SpawnJitterRange.Y; }

	virtual float GetSpawnTraceDistance();
	virtual float GetSpawnTraceOffset();
	virtual float GetSpawnTraceAlpha();

	virtual TEnumAsByte<ETraceTypeQuery> GetSimulationTraceChannel();

	//Generate Meshes
public:
	/* The mesh to generate/modify. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshSelection")
		ESH_MeshGenerationType SelectedMesh = ESH_MeshGenerationType::W2_MG_Singular;

	/* Limit the mesh generation to specific paths only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenerationSettings", AdvancedDisplay)
		bool bGenerateMeshForPathsInRange = false;
#endif
	/* Limit the mesh generation to specific paths only. */
	UPROPERTY(EditAnywhere, Category = "GenerationSettings", AdvancedDisplay, meta = (EditCondition = "bGenerateMeshForPathsInRange"))
		FIntVector2 PathRange = FIntVector2(0);

	UFUNCTION(BlueprintPure, Category = "Debug")
		int32 GetPathRangeX() { return PathRange.X; }
	UFUNCTION(BlueprintPure, Category = "Debug")
		int32 GetPathRangeY() { return PathRange.Y; }
	UFUNCTION(BlueprintCallable, Category = "Debug")
		void SetPathRange(int32 X, int32 Y)
	{
		PathRange = FIntVector2(X, Y);
	}
#if WITH_EDITORONLY_DATA

	/* Make sure the index isnt invalid. */
	virtual int32 GetPathRangeMinChecked();
	/* Make sure the index isnt invalid. */
	virtual int32 GetPathRangeMaxChecked(int32 LastIndex);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenerationSettings", AdvancedDisplay)
		bool bForceIncludeHits = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenerationSettings", AdvancedDisplay, meta = (EditCondition = "bForceIncludeHits"))
		float ForceIncludeHitsTheshold = 60.f;
	/* Maximum number of caching operations on a path per tick to spread the work out across multiple ticks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenerationSettings")
		int32 CachingSubsteps = 50;

	/* How long each segment on the path should be. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshSettings")
		float SegmentLength = 40.f;
	/* The maximum number of segments each path should have. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshSettings")
		int32 MaxSegments = 100;
	/* Limit the spline sample between distance 0:1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshSettings", meta = (ClampMin = "0", ClampMax = "1"))
		FVector2D SamplePath = FVector2D(0.f, 1.f);

	/* Gets the sample path minimum variable checking for whether x or y is the smaller number. */
	virtual float GetSamplePathMinChecked();
	/* Gets the sample path maximum variable checking for whether x or y is the larger  number. */
	virtual float GetSamplePathMaxChecked();

	//Number of times the post-processing smoothing will be applied to cached waterfall positions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions", meta = (ClampMin = "0"))
		int32 SmoothingIterations = 1;
	//How much smoothing to apply in each iteration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		FVector PositionSmoothingPerAxis = FVector(0.25, 0.25, 1.f);
	/* 0:1 for how much the mesh should be projected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions|MeshTrace")
		UCurveFloat* MeshTraceAlpha = nullptr;
	/* Maximum distance for a vertex to trace for nearby obstacle checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions|MeshTrace")
		float MeshTraceDistance = 0.f;
	/* How far a vertex is offset from a surface if the trace returns true (uses hit normal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions|MeshTrace")
		float MeshTraceOffset = 5.f;
	/* Whether the vertices should be placed at the end of the obstacle traces regardless of whether it hit ot not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions|MeshTrace")
		bool bMeshTraceProjection = false;
	/* No Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions|MeshTrace")
		ESH_MeshProjectionType MeshTraceMode = ESH_MeshProjectionType::W2_MP_Positive;

	/*The geometry scripting can auto compute the normals, discarding all the normals that were computed manually.
	Note: normals will still be calculated for tangent and position calculations, they'll just be discarded at the very end when the actual mesh is generated.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals")
		bool bAutoComputeNormals = false;
	/* Whether to flip the normals on the generated dynamic meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals")
		bool bFlipNormals = false;
	/* Offset to add to the generated dynamic mesh normals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals")
		float MeshNormalOffset = 0.f;
	/* Offset to add to the generated dynamic mesh normals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals", AdvancedDisplay)
		UCurveFloat* MeshNormalOffsetCurve = nullptr;
	/* How many smoothing iterations should be performed on the dynamic mesh normals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals")
		int32 SmoothNormalsIteration = 4;
	/* Percentage of smoothed normals to apply to the normals of the dynamic meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals", meta = (ClampMin = "0", ClampMax = "1"))
		float SmoothNormalsAlpha = 1.f;
	/* Treat the normals in a curve like a cylinder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals")
		float CylinderNormal = 0.f;
	/* Treat the normals in a curve like a cylinder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals", AdvancedDisplay)
		UCurveFloat* CylinderNormalCurve = nullptr;
	/* The alpha for how much the Override Normal Direction should be applied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals", AdvancedDisplay)
		float OverrideNormal = 0.f;
	/* The direction of the overwritten normals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals", AdvancedDisplay)
		FVector OverrideNormalDirection = FVector(0.f, 0.f, 1.f);
	/* The alpha of how much to force the normals towards (0, 0, 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normals", AdvancedDisplay)
		float ForceWaterNormalDistZ = 0.f;

	/* How many smoothing iterations should be performed on the dynamic mesh tangents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tangents")
		int32 SmoothTangentsIteration = 4;
	/* Percentage of smoothed tangents to apply to the tangents of the dynamic meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tangents", meta = (ClampMin = "0", ClampMax = "1"))
		float SmoothTangentsAlpha = 1.f;

	/* The scale of the UVs of the generated dynamic meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs")
		FVector2D BaseUVsScale = FVector2D(1.f, 1.f);

	/* Include the kill (splash) impact in the turbulance calculation for the last points in the paths. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence")
		bool bTurbulenceIncludesSplash = false;
	/* Remaps the hit force to a 0:1 range in order to then convert it to turbulence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence")
		float TurbulenceRange = 400.f;
	/* How much obstacle collision calculations affect turbulence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence")
		float TurbulenceObstacleWeight = 3.f;
	/* How much flow calculations  affect turbulence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence")
		float TurbulenceFlowWeight = 100.f;
	/* How far turbulence is added upstream. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence")
		float TurbulenceSpreadUpStream = 300.f;
	/* How far turbulence is added downstream. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence")
		float TurbulenceSpreadDownStream = 800.f;
	/* How fast turbulence increases when added upstream/downstream. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence")
		float TurbulenceFalloff = 4.f;

	/*If the static mesh has been baked and set, update the material for the slot too
	Note: This will ignore any combined slots
	Note: This only sets them in the level, not in the Static Mesh asset*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
		bool bUpdateMaterialsInBakedMeshToo = true;

	/* The material for the generated singular mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
		UMaterialInterface* SingularMeshMaterial = nullptr;
	/* Whether to try sorting points in the singular mesh so that overlapping paths don't generate weird geo.
	Note: This feature is not perfect yet and doesn't always work in 100% of test cases.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SingularMeshSettings", meta = (DisplayName = "EXPERIMENTAL: Sort Points"))
		bool bSortPoints = false;
	/* Whether to subdivide the singular mesh or just use the path data for vertex generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SingularMeshSettings", meta = (DisplayName = "Subdivide"))
		bool bSingularMeshSubdivide = false;
#endif
	/* The number of subdivisions applied to the singular mesh for vertex generation. */
	UPROPERTY(EditAnywhere, Category = "SingularMeshSettings", meta = (DisplayName = "Subdivisions", EditCondition = "bSingularMeshSubdivide"))
		FIntVector2 SingularMeshSubdivisions = FIntVector2(20);

	UFUNCTION(BlueprintPure, Category = "Debug")
		int32 GetSingularMeshSubdivisionsX() { return SingularMeshSubdivisions.X; }
	UFUNCTION(BlueprintPure, Category = "Debug")
		int32 GetSingularMeshSubdivisionsY() { return SingularMeshSubdivisions.Y; }
	UFUNCTION(BlueprintCallable, Category = "Debug")
		void SetSingularMeshSubdivisions(int32 X, int32 Y)
	{
		SingularMeshSubdivisions = FIntVector2(X, Y);
	}
#if WITH_EDITORONLY_DATA

	/* How points are interpolated between two paths. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SingularMeshSettings", meta = (DisplayName = "Subdivision Spline Interpolation", EditCondition = "bSingularMeshSubdivide"))
		ESH_W2_Interpolation SingularMeshSubdivisionSplineInterpolation = ESH_W2_Interpolation::W2_IN_Cubic;

protected:
	/* Applies additional geometry to each side of the waterfall past the start and end of the Top Spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SingularMeshSettings", AdvancedDisplay, meta = (DisplayName = "Additional Geo"))
		float SingularMeshAdditionalGeo = 0.f;
public:
	/* Applies additional geometry to each side of the waterfall past the start and end of the Top Spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SingularMeshSettings", AdvancedDisplay, meta = (DisplayName = "Additional Geo Curve"))
		UCurveFloat* SingularMeshAdditionalGeoCurve = nullptr;
	/* Applies additional geometry to each side of the waterfall past the start and end of the Top Spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SingularMeshSettings", AdvancedDisplay, meta = (DisplayName = "Additional Geo Spread Curve"))
		UCurveFloat* SingularMeshAdditionalGeoSpreadCurve = nullptr;

	virtual float GetSingularMeshAdditionalGeo();

	/* The material to apply to the PerPath meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
		UMaterialInterface* PerPathMeshMaterial = nullptr;
	/* Width of an individual path's mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PerPathSettings", AdvancedDisplay, meta = (DisplayName = "Width Curve"))
		UCurveFloat* PerPathWidthCurve = nullptr;
	/* Width of an individual path's mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PerPathSettings", meta = (DisplayName = "Width"))
		float PerPathWidth = 120.f;
	/* The subdivisions to use for the individual path meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PerPathSettings", meta = (DisplayName = "Subdivisions"))
		int32 PerPathSubdivisions = 5;
	/* How much the first two vertices can roll to match the Top Spline's slope (otherwise flattened in the Z-axis). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PerPathSettings", AdvancedDisplay, meta = (DisplayName = "Spline Roll", ClampMin = "0", ClampMax = "1"))
		float PerPathSplineRoll = 1.f;

	/* Additional bulge to add to the dynamic mesh.
	* Bulge is used for Per Path, Plane, and Splash Meshes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bulge", meta = (DisplayName = "Bulge"))
		float PerPathBulge = 50.f;
	/* Additional bulge to add to the dynamic mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bulge", AdvancedDisplay, meta = (DisplayName = "Bulge Curve"))
		UCurveFloat* PerPathBulgeCurve = nullptr;
	/* Additional bulge to add to the dynamic mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bulge", AdvancedDisplay, meta = (DisplayName = "Bulge Profile Curve"))
		UCurveFloat* PerPathBulgeProfileCurve = nullptr;
	/* If Bulge Curve == nullptr, whether to apply the bulge to the first row of vertices or to flatten them to conform with the top spline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bulge", meta = (DisplayName = "Flatten First Row Bulge"))
		bool bPerPathFlattenFirstRowBulge = true;

	/* The material for the Plane meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials", meta = (DisplayName = "Plane Mesh Material"))
		UMaterialInterface* CrossMeshMaterial = nullptr;
	/* Width of an individual path's mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlaneMeshSettings", AdvancedDisplay, meta = (DisplayName = "Width Curve"))
		UCurveFloat* CrossPlaneWidthCurve = nullptr;
	/* Width of an individual path's mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlaneMeshSettings", meta = (DisplayName = "Width"))
		float CrossPlaneWidth = 50.f;
	/* The subdivisions to use for the individual path meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlaneMeshSettings", meta = (DisplayName = "Subdivisions"))
		int32 CrossPlaneSubdivisions = 2;
	/* -1:1 Offset applied to the vertices along the path's normal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlaneMeshSettings", AdvancedDisplay, meta = (DisplayName = "Offset Curve"))
		UCurveFloat* CrossPlaneOffsetCurve = nullptr;
	/* -1:1 Offset applied to the vertices along the path's normal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlaneMeshSettings", meta = (DisplayName = "Offset", ClampMin = "-1", ClampMax = "1"))
		float CrossPlaneOffset = 0.1f;
	/* How much the first two vertices can roll to match the Top Spline's slope (otherwise flattened in the Z-axis). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlaneMeshSettings", AdvancedDisplay, meta = (DisplayName = "Spline Roll", ClampMin = "0", ClampMax = "1"))
		float CrossPlaneSplineRoll = 1.f;
	/* Forces the Cross mesh's normals to point towards the path's normal (overrides both OverrideNormal and CylinderNormal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlaneMeshSettings", meta = (DisplayName = "Align Normal", ClampMin = "0", ClampMax = "1"))
		float CrossPlaneAlignNormal = 1.f;

	/* The material used for the generated Splash meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
		UMaterialInterface* SplashMeshMaterial = nullptr;
	/* The subdivisions to use for the individual splash meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplashMeshSettings", meta = (DisplayName = "Subdivisions", EditCondition = "bSplashOverrideSubdivisions"))
		int32 SplashMeshOverrideSubdivisions = 10;
	/* The radius of the front part of the generated splash mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplashMeshSettings", meta = (DisplayName = "Front Radius"))
		float SplashFrontRadius = 300.f;
	/* The radius of the back part of the generated splash mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplashMeshSettings", meta = (DisplayName = "Back Radius"))
		float SplashBackRadius = 75.f;
	/* The subdivisions to use for the generated splash path (circumferance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplashMeshSettings", meta = (DisplayName = "Radius Subdivision"))
		int32 SplashRadiusSubdivisions = 12;
	/* Simple scale of the generated splash meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplashMeshSettings", meta = (DisplayName = "Scale"))
		float SplashScale = 1.f;
	/* How many segments to make between the last point of the path and the outer circumference of the splash mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplashMeshSettings", meta = (DisplayName = "Segments", ClampMin = "-1"))
		int32 SplashSegments = 10;
	/* Distribution of vertices along the distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplashMeshSettings", AdvancedDisplay, meta = (DisplayName = "Distance Falloff"))
		float SplashDistanceFalloff = 1.f;
	/* How UV0 is distributed between the path point and the circumference. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplashMeshSettings", AdvancedDisplay, meta = (DisplayName = "UV Falloff"))
		float SplashUVFalloff = 1.f;

	/* Should the Single mesh be generated? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshGenerationSettings", meta = (DisplayName = "Single"))
		bool SimpleGenerateSingleMesh = true;
	/* Should the Per Path meshes be generated? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshGenerationSettings", meta = (DisplayName = "Paths"))
		bool SimpleGeneratePerPathMesh = true;
	/* Should the Splash mesh be generated? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshGenerationSettings", meta = (DisplayName = "Splash"))
		bool SimpleGenerateSplashMesh = true;
	/* Should the Plane meshes be generated? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshGenerationSettings", meta = (DisplayName = "Planes"))
		bool SimpleGeneratePlaneMesh = true;

	/* Should the Top VFX be generated? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top VFX Settings")
		bool bTopVFX = true;
	/* The Niagara System to use for the Top VFX */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top VFX Settings", meta = (EditCondition = "bTopVFX", DisplayName = "Top VFX System"))
		UNiagaraSystem* TopVfxSystem = nullptr;
	/* Should the Bottom VFX be generated? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bottom VFX Settings")
		bool bBottomVFX = true;
	/* The Niagara System to use for the Bottom VFX */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bottom VFX Settings", meta = (EditCondition = "bBottomVFX", DisplayName = "Bottom VFX System"))
		UNiagaraSystem* BottomVfxSystem = nullptr;
	/* Should the Middle VFX be generated? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Middle VFX Settings")
		bool bMiddleVFX = true;
	/* The Niagara System to use for the Middle VFX */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Middle VFX Settings", meta = (EditCondition = "bMiddleVFX", DisplayName = "Middle VFX System"))
		UNiagaraSystem* MiddleVfxSystem = nullptr;
	/* Whether the Middle VFX uses a Z-Distance to calculate where the VFX points on the waterfall are, or whether it uses spline points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Middle VFX Settings")
		bool bMiddleUsesDistance = true;
#endif
	/* The spline point range for all paths on the waterfall to use for the Middle VFX points. */
	UPROPERTY(EditAnywhere, Category = "Middle VFX Settings")
		FIntVector2 PointRange = FIntVector2(1, 2);

	UFUNCTION(BlueprintPure, Category = "Debug")
		int32 GetPointRangeX() { return PointRange.X; }
	UFUNCTION(BlueprintPure, Category = "Debug")
		int32 GetPointRangeY() { return PointRange.Y; }
	UFUNCTION(BlueprintCallable, Category = "Debug")
		void SetPointRange(int32 X, int32 Y)
	{
		PointRange = FIntVector2(X, Y);
	}
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Debug")
		bool bShowArrowComponents = true;
	UPROPERTY(EditAnywhere, Category = "Debug")
		float ArrowComponentLength = 80.0f;

	/* The world-space z-translation range to add the Middle FX from highest z-point to lowest z-point.
	* Eg, if DistanceRange is 0.3 to 0.8, and the highest point is 100, the lowest 0, then it will find where each path crosses 30 and 80 in world space z
	* and use this as the range to generate the points for the Middle VFX. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Middle VFX Settings", meta = (ClampMin = "0", ClampMax = "1"))
		FVector2D DistanceRange = FVector2D(0.3f, 0.8f);
	/* The number of points to map on each path between DistanceRange (inclusive).
	* This defaults to 2 if the number is < 2.
	* Eg, if RemapPoints is 5, Point[0] will be DistanceRange.Max and Point[4] will be DistanceRange.Min,
	* with the other three points distributed evenly along the spline path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Middle VFX Settings")
		int32 RemapPoints = 5;

	/* Which path is currently selected for FX editing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX (Individual Points)", meta = (ClampMin = "0"))
		int32 FxPathSelected = 0;

	/* Returns the correct material based on SelectedMesh. */
	virtual UMaterialInterface* GetMaterialForMesh();

	virtual void UpdateCurrentTabState(ESH_W2_TabState NewTabState);
	virtual ESH_W2_TabState GetCurrentTabState() { return CurrentTabState; }

protected:
	ESH_W2_TabState CurrentTabState = ESH_W2_TabState::W2_TS_Simple;

public:
	FSimpleDelegate RefreshDetailsPanel = FSimpleDelegate();

	//Whether to combine the same materials into one material slot or keep them separate per original mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BakeSettings")
		bool bCombineSameMaterials = false;

protected:
	virtual void UpdateMaterialForComponent(ESH_MeshGenerationType MeshType, UMaterialInterface* NewMaterial);

	//~ Begin UObject Interface
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

protected:
	//Generate Paths
	virtual void StartSimulate(FSimpleDelegate OnProcessingFinished = FSimpleDelegate());
	virtual void CancelSimulate();

	//Generate Meshes
	virtual void StartMeshGeneration(FSimpleDelegate OnProcessingFinished = FSimpleDelegate());
	virtual void CancelMeshGeneration();
	virtual void ClearDynamicMeshes(ESH_MeshGenerationType MeshType = ESH_MeshGenerationType::W2_MG_All, FSimpleDelegate OnProcessingFinished = FSimpleDelegate());

	//Bake to Mesh
	virtual void BakeToStaticMesh(FSimpleDelegate OnProcessingFinished = FSimpleDelegate());

public:
	virtual bool IsSimple();
	virtual bool HasGeneratedMeshes();

public:
	/* The last path targeted in the Content Browser for Static Mesh generation. */
	UPROPERTY()
		FString LastBakePath = "/Game/";
#endif //WITH_EDITORONLY_DATA
};
