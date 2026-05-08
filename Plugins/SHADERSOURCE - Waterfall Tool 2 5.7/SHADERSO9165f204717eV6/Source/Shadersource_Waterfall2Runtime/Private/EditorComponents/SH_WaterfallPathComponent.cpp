// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "EditorComponents/SH_WaterfallPathComponent.h"

#if WITH_EDITOR
#include "EditorComponents/SH_WaterfallSettingsComponent.h"
#include "SH_WaterfallTool2Statics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/BoxComponent.h"
#include "GeometryScript/ShapeFunctions.h"
#include "Actors/SH_Waterfall2.h"
#endif //WITH_EDITOR

USH_WaterfallPathComponent::USH_WaterfallPathComponent()
{
	//This component doesn't need to tick - the ticking is handled by the SH_WaterfallGenerationComponent
	PrimaryComponentTick.bCanEverTick = false;
}

#if WITH_EDITOR
ASH_Waterfall2* USH_WaterfallPathComponent::GetParentWaterfall()
{
	if (!ParentWaterfall)
	{
		ParentWaterfall = GetOwner<ASH_Waterfall2>();
	}

	return ParentWaterfall;
}

USH_WaterfallSettingsComponent* USH_WaterfallPathComponent::GetWaterfallSettings()
{
	if (!SettingsComponent)
	{
		if (GetParentWaterfall())
		{
			SettingsComponent = GetParentWaterfall()->GetWaterfallSettings();
		}
	}

	return SettingsComponent;
}

void USH_WaterfallPathComponent::CalculateIntialSimulationVariables()
{
	GeneratedPathSeed = GetWaterfallSettings()->Random().FRand();

	//Find the initial position - we're working in world space here for the trace
	FVector Up = GetUpVector();
	FVector UpXDistance = Up * GetWaterfallSettings()->GetSpawnTraceDistance();
	FVector UpXOffset = Up * GetWaterfallSettings()->GetSpawnTraceOffset();

	FHitResult OutHit;
	UKismetSystemLibrary::LineTraceSingle(
		this,
		GetComponentLocation() + UpXDistance + UpXOffset,
		GetComponentLocation() - UpXDistance - UpXOffset,
		GetWaterfallSettings()->GetSimulationTraceChannel(),
		true,
		{},
		EDrawDebugTrace::None,
		OutHit,
		true
	);

	FVector InitialPosition = (OutHit.bBlockingHit)
		? FMath::Lerp(FVector(0.f, 0.f, 0.f), OutHit.ImpactPoint + (OutHit.ImpactNormal * GetWaterfallSettings()->GetSpawnTraceOffset()), GetWaterfallSettings()->GetSpawnTraceAlpha())
		: GetComponentLocation();
	
	//Calculate the initial speed
	float InitialSpeed = FMath::Clamp(GetWaterfallSettings()->Speed + GetWaterfallSettings()->Random().FRandRange(GetWaterfallSettings()->SpeedRandom * -1.f, GetWaterfallSettings()->SpeedRandom), 0.f, GetWaterfallSettings()->MaxSpeed);

	ResetSimulationVariables();
	
	float Direction = (GetWaterfallSettings()->bFlipSplineDirection) ? -1.f : 1.f;
	AddNewSimulatedPoint(InitialPosition, FSH_SimulatedPoint(GetForwardVector() * Direction * InitialSpeed, ESH_SimulatedPointState::Start));
}

//Get the particle's projected direction based on its hit normal
FVector USH_WaterfallPathComponent::GetProjectedDirectionFromHitNormal(FSH_SimulatedPoint CurrentPoint, FVector ImpactNormal, float MinSpeed)
{
	FVector ProjectedDirection = FVector::ZeroVector;

	FVector ProjectedVelocity = FVector::VectorPlaneProject(CurrentPoint.Velocity, ImpactNormal);
	float ProjectedVelocityLength = ProjectedVelocity.Size();
	if (ProjectedVelocityLength > MinSpeed)
	{
		ProjectedDirection = ProjectedVelocity / ProjectedVelocityLength;
	}
	else
	{
		//If the projected velocity is too close to zero, give it a random direction to simulate the free flow of fluid (rather than have it bounce directly up)
		ProjectedDirection = FVector(1.f, 0.f, 0.f).RotateAngleAxis(GetWaterfallSettings()->Random().FRandRange(-180.f, 180.f), ImpactNormal);
	}

	return ProjectedDirection;
}

bool USH_WaterfallPathComponent::SimulatePath(float DeltaTime, int32& Iteration, TArray<USH_WaterfallPathComponent*> PathsForDeviation)
{
	if (SimulatedPoints.Num() <= 0)
	{
		Iteration = GetWaterfallSettings()->GetSubsteps();
	}
	else
	{
		FSH_SimulatedPoint CurrentPoint = SimulatedPoints[SimulatedPoints.Num() - 1];
		FVector CurrentPointPosition = GetSimulatedPointPosition(SimulatedPoints.Num() - 1);

		//While we still have iterations left for this frame
		while (Iteration < GetWaterfallSettings()->GetSubsteps())
		{
			Iteration++;

			FSH_SimulatedPoint PreviousPointState = CurrentPoint;
			FVector PreviousPointPosition = CurrentPointPosition;

			//Apply movement--------
			{
				FVector InitialVelocity = CurrentPoint.Velocity;

				//Add gravity
				CurrentPoint.Velocity += FVector(0.f, 0.f, GetWaterfallSettings()->Gravity * DeltaTime);

				//Apply the Speed Limit
				CurrentPoint.Velocity = UKismetMathLibrary::ClampVectorSize(CurrentPoint.Velocity, 0.f, GetWaterfallSettings()->MaxSpeed);

				//Add drag
				if (GetWaterfallSettings()->Drag > 0.f)
				{
					CurrentPoint.Velocity *= 1.f - FMath::Clamp(GetWaterfallSettings()->Drag * DeltaTime, 0.f, 1.f);
				}

				//Apply velocity using verlet mid point integration method
				CurrentPointPosition += (CurrentPoint.Velocity + InitialVelocity) * 0.5f * DeltaTime;
			}

			//Check for Kill Box intersections
			bool bKilled = false;
			float ClosestHitPositionZ = 0.f;
			{
				float ClosestHitMagnitude = -1.f;
				FVector ClosestHitPosition = FVector::ZeroVector;

				/* Loop through all of the kill boxes to perform segment-box intersection tests.
				* This will make sure it hasn't passed through a kill box during its movement,
				* rather than just checking if the point is intersecting one now because it might've
				* passed through one during its movement between frames.*/

				float Distance = FVector::Distance(CurrentPointPosition, PreviousPointPosition);
				FVector Direction = (CurrentPointPosition - PreviousPointPosition) / Distance;

				auto KillParticle = [&]()
				{
					//Alpha is the actual travelled distance until the hit point
					CurrentPoint.Velocity = FMath::Lerp(PreviousPointState.Velocity, CurrentPoint.Velocity, FVector::Distance(CurrentPointPosition, PreviousPointPosition) / Distance);

					//Output hit normal and force
					CurrentPoint.HitNormal = FVector(0.f, 0.f, 1.f); //Assume the water plane is flat
					CurrentPoint.HitForce = FMath::Abs(FVector::DotProduct(CurrentPoint.HitNormal, CurrentPoint.Velocity));

					//Flag the particle as killed
					CurrentPoint.State = ESH_SimulatedPointState::Killed;

					bKilled = true;
				};

				//Check for collision with the Kill Plane - this trace is *infinite* so there are no bounds based on x-y size of the plane
				if (Distance > 0.f)
				{
					//UGeometryScriptLibrary_RayFunctions::GetRayPlaneIntersection has been changed in 5.4 and it breaks the collision checks here
					//so this is the function copied from 5.3
					auto GetRayPlaneIntersection = [](FRay Ray, FPlane Plane, double& HitDistance) -> bool
					{
						const FVector PlaneNormal = FVector(Plane.X, Plane.Y, Plane.Z);
						const FVector PlaneOrigin = PlaneNormal * Plane.W;
						if (FMathd::Abs(FVector::DotProduct(Ray.Direction, PlaneNormal)) > FMathd::ZeroTolerance)
						{
							HitDistance = FVector::DotProduct((PlaneOrigin - Ray.Origin), PlaneNormal);
							return true;
						}
						HitDistance = 0;
						return false;
					};

					//If the ray passes through the plane - this is tracing *infinitely* so that it can keep track of objects further on and predict upcoming obstacles/death
					//The plane normal needs to be -Z, otherwise the hit distance ends up negative for some reason, even though theoretically distance should never be negative
					double HitDistance = 0.f;
					if (GetRayPlaneIntersection(FRay(PreviousPointPosition, Direction), UKismetMathLibrary::MakePlaneFromPointAndNormal(GetParentWaterfall()->GetKillPlane()->GetComponentLocation(), FVector(0.f, 0.f, -1.f)), HitDistance))
					{
						FVector HitPosition = PreviousPointPosition + (Direction * HitDistance);
					
						//If the segment is hit it means that the particle has gone through this kill floor. We then need to calculate the velocity at the hit.
						if (HitDistance <= Distance)
						{
							CurrentPointPosition = HitPosition;
					
							KillParticle();
						}
						else
						{
							//Keep track of the ray so that we can predict when we hit the water plane
							float VectorSizeSquared = HitPosition.SizeSquared();
							if (VectorSizeSquared < ClosestHitMagnitude || ClosestHitMagnitude < 0.f)
							{
								ClosestHitMagnitude = VectorSizeSquared;
								ClosestHitPosition = HitPosition;
							}
						}
					}
				}

				ClosestHitPositionZ = ClosestHitPosition.Z;

				//Check the other kill actors
				if (!bKilled && GetWaterfallSettings()->AdditionalKillActors.Num() > 0)
				{
					TArray<FHitResult> HitResults = {};
					if (UKismetSystemLibrary::LineTraceMulti(
						this,
						PreviousPointPosition,
						CurrentPointPosition + (FVector(20000.f) * Direction), //Like above, the trace needs to happen past the end point position so keep track of foreseeing collisions in the future
						GetWaterfallSettings()->GetSimulationTraceChannel(),
						true,
						{},
						EDrawDebugTrace::None,
						HitResults,
						true
					))
					{
						for (const FHitResult& HitResult : HitResults)
						{
							bool bFoundActor = false;
							for (AActor* KillActor : GetWaterfallSettings()->AdditionalKillActors)
							{
								if (HitResult.GetActor() == KillActor)
								{
									//If the segment is hit it mean that the particle has gone through this kill floor. We then need to calculate the velocity at the hit.
									if (HitResult.Distance <= Distance)
									{
										CurrentPointPosition = HitResult.ImpactPoint;

										//For some reason X is offset in the HitLocation, so bandaid fix here is to override X
										CurrentPointPosition.X = PreviousPointPosition.X;

										KillParticle();
									}
									else
									{
										//Keep track of the ray so that we can predict when we hit the water plane
										float VectorSizeSquared = HitResult.ImpactPoint.SizeSquared();
										if (VectorSizeSquared < ClosestHitMagnitude || ClosestHitMagnitude < 0.f)
										{
											ClosestHitMagnitude = VectorSizeSquared;
											ClosestHitPosition = HitResult.ImpactPoint;
										}
									}

									bFoundActor = true;
									break;
								}
							}

							if (bFoundActor)
							{
								ClosestHitPositionZ = ClosestHitPosition.Z;
								break;
							}
						}
					}
				}
			}

			if (!bKilled) //We now need to do collision checks
			{
				//Apply obstacle collision
				{
					//If the particle is blocked by something, figure out the actual position at the end of the frame, rather than the expected position
					FVector PreviousPosition = PreviousPointPosition;
					float RemainingDeltaTimePercent = 1.f;
					int32 IterationCount = 0;
					const int32 IterationCountMAX = 10; //Max count in case there are too many little collisions

					while (RemainingDeltaTimePercent > 0.f && IterationCount < IterationCountMAX)
					{
						//Trace between the previous and current particle positions
						FHitResult OutHit;
						if (UKismetSystemLibrary::LineTraceSingle(
							this,
							PreviousPosition,
							CurrentPointPosition,
							GetWaterfallSettings()->GetSimulationTraceChannel(),
							true,
							GetWaterfallSettings()->IgnoreActorsForAvoidance,
							EDrawDebugTrace::None,
							OutHit,
							true
						))
						{
							if (OutHit.Location.Z < ClosestHitPositionZ)
							{
								//If it's the first iteration, then the particle is airborn since there was no hit at all
								if (IterationCount == 0)
								{
									CurrentPoint.State = ESH_SimulatedPointState::Airborn;
									CurrentPoint.HitForce = 0.f;
									RemainingDeltaTimePercent = 0.f;
								}
							}
							else //it passed the kill floor check
							{
								CurrentPoint.HitNormal = OutHit.ImpactNormal;
								CurrentPoint.HitForce = FMath::Abs(FVector::DotProduct(OutHit.ImpactNormal, CurrentPoint.Velocity));

								//This particle would be a sliding particle if the previous point has a hit normal similar to this one
								CurrentPoint.State = (
									(PreviousPointState.State == ESH_SimulatedPointState::Hit || PreviousPointState.State == ESH_SimulatedPointState::Slide)
									&& PreviousPointState.HitNormal.Equals(CurrentPoint.HitNormal, 0.001f)
									)
									? ESH_SimulatedPointState::Slide
									: ESH_SimulatedPointState::Hit
									;

								//Get the particle's projected direction based on its hit normal
								FVector ProjectedDirection = GetProjectedDirectionFromHitNormal(CurrentPoint, CurrentPoint.HitNormal);

								//Apply frictional force
								FVector PreFrictionalVelocity = CurrentPoint.Velocity;

								CurrentPoint.Velocity -= ProjectedDirection * FVector::DotProduct(PreFrictionalVelocity, ProjectedDirection) * FMath::Clamp(RemainingDeltaTimePercent * DeltaTime * GetWaterfallSettings()->Friction, 0.f, 1.f);
							
								//Make sure friction didn't reverse the movement
								if (FVector::DotProduct(CurrentPoint.Velocity, PreFrictionalVelocity) < 0.f)
								{
									CurrentPoint.Velocity = FVector::ZeroVector;
								}

								//Distribute energy along the movement
								CurrentPoint.Velocity = FMath::Lerp(ProjectedDirection * CurrentPoint.Velocity.Size(), FMath::GetReflectionVector(CurrentPoint.Velocity, CurrentPoint.HitNormal), FMath::Clamp(GetWaterfallSettings()->Bounce, 0.f, 1.f));

								//Check if the particle has stalled
								if (CurrentPoint.Velocity.SizeSquared() <= FMath::Square(GetWaterfallSettings()->KillSpeed))
								{
									CurrentPointPosition = OutHit.TraceEnd;
									CurrentPoint.Velocity = FVector::ZeroVector;
									CurrentPoint.State = ESH_SimulatedPointState::Stalled;
									RemainingDeltaTimePercent = 0.f;
								}
								else
								{
									//Nudge the point away from objects
									FHitResult OutHitNudge;
									if (UKismetSystemLibrary::LineTraceSingle(
										this,
										OutHit.ImpactPoint + 0.000001f,
										OutHit.ImpactPoint + (OutHit.ImpactNormal * 0.01f),
										GetWaterfallSettings()->GetSimulationTraceChannel(),
										true,
										GetWaterfallSettings()->IgnoreActorsForAvoidance,
										EDrawDebugTrace::None,
										OutHitNudge,
										true
									))
									{
										//If true then this particle is blocked
										CurrentPointPosition = OutHitNudge.TraceStart;
										CurrentPoint.Velocity = FVector::ZeroVector;
										CurrentPoint.State = ESH_SimulatedPointState::Blocked;
										RemainingDeltaTimePercent = 0.f;
									}
									else
									{
										//We've hit something but we haven't become stalled or blocked, so we need to calculate the remaining delta time left
										RemainingDeltaTimePercent = FMath::Clamp(RemainingDeltaTimePercent - OutHit.Time, 0.f, 1.f);
										PreviousPosition = OutHitNudge.TraceEnd;
										CurrentPointPosition = PreviousPosition + (CurrentPoint.Velocity * (RemainingDeltaTimePercent * DeltaTime));
									}
								}
							}
						}
						else //There was no hit
						{
							//If it's the first iteration, then the particle is airborn since there was no hit at all
							if (IterationCount == 0)
							{
								CurrentPoint.State = ESH_SimulatedPointState::Airborn;
								CurrentPoint.HitForce = 0.f;
								RemainingDeltaTimePercent = 0.f;
							}
						}

						IterationCount++;
					}
				}

				//Apply obstacle deviation
				{
					CurrentPoint.ObstacleDeviation = 0.f;

					float CalculatedDeviation = 0.f;
					float CalculatedRepulsion = 0.f;

					//Trace ahead for upcoming obstacles and apply a deviating force
					FHitResult OutHitDeviation;
					if (UKismetSystemLibrary::LineTraceSingle(
						this,
						CurrentPointPosition,
						CurrentPointPosition + (CurrentPoint.Velocity * GetWaterfallSettings()->ObstacleAvoidanceDistance),
						GetWaterfallSettings()->GetSimulationTraceChannel(),
						true,
						GetWaterfallSettings()->IgnoreActorsForAvoidance,
						EDrawDebugTrace::None,
						OutHitDeviation,
						true
					))
					{
						//Don't worry about hits below the kill z plane
						if (OutHitDeviation.ImpactPoint.Z >= ClosestHitPositionZ)
						{
							//Calculate the deviation value
							CalculatedDeviation = FMath::Clamp(FMath::Pow(1.f - OutHitDeviation.Time,GetWaterfallSettings()->ObstacleAvoidanceFalloff) * GetWaterfallSettings()->ObstacleAvoidanceWeight * DeltaTime /* * Alpha = 1.f*/, 0.f, 1.f);
						
							//Apply the deviation
							CurrentPoint.Velocity = USH_WaterfallTool2Statics::SlerpNormals(CurrentPoint.Velocity.GetSafeNormal(0.0001f), GetProjectedDirectionFromHitNormal(CurrentPoint, OutHitDeviation.ImpactNormal, 10.f), CalculatedDeviation) * CurrentPoint.Velocity.Size();
						}
					}

					//Trace ahead for upcoming obstacles and apply a repulsion force
					FHitResult OutHitRepulsion;
					if (UKismetSystemLibrary::LineTraceSingle(
						this,
						CurrentPointPosition,
						CurrentPointPosition + (CurrentPoint.Velocity * GetWaterfallSettings()->ObstacleRepulsionDistance),
						GetWaterfallSettings()->GetSimulationTraceChannel(),
						true,
						GetWaterfallSettings()->IgnoreActorsForAvoidance,
						EDrawDebugTrace::None,
						OutHitRepulsion,
						true
					))
					{
						//Don't worry about hits below the kill z plane
						if (OutHitRepulsion.ImpactPoint.Z >= ClosestHitPositionZ)
						{
							//Calculate the repulsion value
							CalculatedRepulsion = FMath::Clamp(FMath::Pow(1.f - OutHitRepulsion.Time, GetWaterfallSettings()->ObstacleRepulsionFalloff) * GetWaterfallSettings()->ObstacleRepulsionWeight * DeltaTime /* * Alpha = 1.f*/, 0.f, 1.f);

							//Apply the repulsion
							CurrentPoint.Velocity = CurrentPoint.Velocity.GetSafeNormal(0.0001f)* (CurrentPoint.Velocity.Size()* (1 - CalculatedRepulsion));
						}
					}

					//Update the deviation value
					CurrentPoint.ObstacleDeviation = FMath::Clamp(FMath::Lerp(CalculatedDeviation, CalculatedRepulsion, 0.5), 0.f, 1.f);
				}

				//Apply flow deviation (particle velocity affected by other nearby simulated particles)
				{
					float Alpha = 1.f - CurrentPoint.ObstacleDeviation;

					CurrentPoint.FlowDeviation = 0.f;

					if (Alpha > 0.f
						&& ((GetWaterfallSettings()->FlowRepulsionWeight > 0.f && GetWaterfallSettings()->FlowRepulsionDistance > 0.f)
							|| (GetWaterfallSettings()->FlowAlignmentWeight && GetWaterfallSettings()->FlowAlignmentDistance > 0.f))
						)
					{
						FVector RepulsionAlignmentAmount = FVector::ZeroVector;
						FVector RepulsionForceAmount = FVector::ZeroVector;

						for (USH_WaterfallPathComponent* Path : PathsForDeviation)
						{
							if (Path != this)
							{
								// Is this point close enough to check to the path's overall bounding box to proceed with the checks?
								if (Path->IsInBounds(CurrentPointPosition, GetWaterfallSettings()->FlowAlignmentDistance))
								{
									FSH_SimulatedPoint FoundPoint;
									if (FindNearestPoint(CurrentPointPosition, FoundPoint))
									{
										FVector FoundPointPosition = GetPointPosition(FoundPoint.PointIndex);

										//Figure out flow alignment and flow repulsion

										float Dist = FVector::Dist(CurrentPointPosition, FoundPointPosition);

										if (GetWaterfallSettings()->FlowAlignmentWeight && GetWaterfallSettings()->FlowAlignmentDistance > 0.f)
										{
											RepulsionAlignmentAmount += FoundPoint.Velocity * (GetWaterfallSettings()->FlowAlignmentWeight * GetWaterfallSettings()->GetSimulationDeltaTime()) * FMath::Pow(1 - FMath::Clamp(Dist / GetWaterfallSettings()->FlowAlignmentDistance, 0.f, 1.f), GetWaterfallSettings()->FlowAlignmentFalloff);
										}

										if (GetWaterfallSettings()->FlowRepulsionWeight && GetWaterfallSettings()->FlowRepulsionDistance > 0.f)
										{
											RepulsionForceAmount += (CurrentPointPosition - FoundPointPosition).GetSafeNormal(0.0001f) * (GetWaterfallSettings()->FlowRepulsionWeight * GetWaterfallSettings()->GetSimulationDeltaTime()) * FMath::Pow(1 - FMath::Clamp(Dist / GetWaterfallSettings()->FlowRepulsionDistance, 0.f, 1.f), GetWaterfallSettings()->FlowRepulsionFalloff);
										}
									}
								}
							}
						}

						//We need to make sure that the new velocity is not greater than the previous velocity
						FVector PrevVel = CurrentPoint.Velocity;
						float PrevSpeed = PrevVel.Size();
						CurrentPoint.Velocity = (PrevVel + RepulsionForceAmount + RepulsionAlignmentAmount).GetClampedToSize(0.f, PrevSpeed);

						//Figure out flow deviation based on speed and angle
						PrevSpeed = FMath::Max(PrevSpeed, 1.f);
						float CalcSpeed = FMath::Abs(CurrentPoint.Velocity.Size() - PrevSpeed) / PrevSpeed * 0.5f;
						float CalcAngle = ((FVector::DotProduct(CurrentPoint.Velocity.GetSafeNormal(0.0001f), PrevVel / PrevSpeed) * -1.f) + 1.f) * 0.5f;
						CurrentPoint.FlowDeviation = CalcSpeed + CalcAngle;
					}
				}

				//Check kill speed
				{
					if (CurrentPoint.State != ESH_SimulatedPointState::Start
						&& CurrentPoint.State != ESH_SimulatedPointState::Airborn
						&& CurrentPoint.State != ESH_SimulatedPointState::Blocked
						&& CurrentPoint.Velocity.SizeSquared() < FMath::Square(GetWaterfallSettings()->KillSpeed))
					{
						CurrentPoint.Velocity = FVector::ZeroVector;
						CurrentPoint.State = ESH_SimulatedPointState::Stalled;
					}
				}
			}

			//Update distance and age
			CurrentPoint.Distance += FVector::Distance(PreviousPointPosition, CurrentPointPosition); //Distance travelled overall so far
			CurrentPoint.Age += DeltaTime;

			if (CurrentPoint.Age > GetWaterfallSettings()->Lifetime)
			{
				//Set to died unless it's already gone through a kill floor
				if (CurrentPoint.State != ESH_SimulatedPointState::Killed) CurrentPoint.State = ESH_SimulatedPointState::Died;
			}

			//SimulatedPoints.Add(CurrentPoint);
			AddNewSimulatedPoint(CurrentPointPosition, CurrentPoint);

			//Calculate bounds and variables for use later
			SimulationBounds = FBox(
				FVector(//Min
					FMath::Min(SimulationBounds.Min.X, CurrentPointPosition.X),
					FMath::Min(SimulationBounds.Min.Y, CurrentPointPosition.Y),
					FMath::Min(SimulationBounds.Min.Z, CurrentPointPosition.Z)
				),
				FVector(//Max
					FMath::Max(SimulationBounds.Max.X, CurrentPointPosition.X),
					FMath::Max(SimulationBounds.Max.Y, CurrentPointPosition.Y),
					FMath::Max(SimulationBounds.Max.Z, CurrentPointPosition.Z)
				)
			);

			SimulationMaxHitForce = FMath::Max(SimulationMaxHitForce, CurrentPoint.HitForce);
			SimulationMaxSpeed = FMath::Max(SimulationMaxSpeed, CurrentPoint.Velocity.Size());

			//If the particle is no longer still alive
			if (CurrentPoint.State >= ESH_SimulatedPointState::Stalled)
			{
				bIsSimulated = true;
				return true;
			}
		}
	}

	return false;
}

void USH_WaterfallPathComponent::ResetSimulationVariables()
{
	bIsSimulated = false;
	SimulatedPoints = {};
	SimulationBounds = FBox();
	SimulationMaxHitForce = 0.f;
	SimulationMaxSpeed = 0.f;
}

bool IsValidIndex(int Index, TArray<FSH_SimulatedPoint> InArray)
{
	return Index >= 0 && Index < InArray.Num();
}

void USH_WaterfallPathComponent::SetSampleLength(float SuggestedLength, int32 MinSamples, int32 MaxSamples)
{
	float PathLength = GetSimulationDistance() * (GetWaterfallSettings()->GetSamplePathMaxChecked() - GetWaterfallSettings()->GetSamplePathMinChecked());

	int32 SampleCount = 0;
	if (PathLength > 0.f && SuggestedLength > 0.f)
	{
		SampleCount = FMath::Clamp(FMath::CeilToInt(PathLength / FMath::Min(PathLength, SuggestedLength)), FMath::Min(MaxSamples, MinSamples), FMath::Max(MaxSamples, MinSamples));
	}
	else
	{
		SampleCount = FMath::Max(MaxSamples, MinSamples);
	}

	float SampleLength = PathLength / FMath::Max<float>(SampleCount - 1.f, 1.f);

	if (SampleCount != CalculatedSampleCount || SampleLength != CalculatedSampleLength)
	{
		CalculatedSampleCount = SampleCount;
		CalculatedSampleLength = SampleLength;
		
		InvalidateSampleCache();
		InvalidatePointsCache();
	}
}

void USH_WaterfallPathComponent::SetSampleIncludeHits(bool bIncludeHits, float Threshold)
{
	bSampleIncludeHits = bIncludeHits;
	SampleThreshold = Threshold;
}

float USH_WaterfallPathComponent::GetSimulationDistance()
{
	return (SimulatedPoints.Num() > 0) ? SimulatedPoints[SimulatedPoints.Num() - 1].Distance : -1.f;
}

float USH_WaterfallPathComponent::GetSimulationBoundsHeight()
{
	return FMath::Max(SimulationBounds.Max.Z - SimulationBounds.Min.Z, 1.f); //Should be at least 1 so that there's never a divide by 0
}

void USH_WaterfallPathComponent::InvalidateSampleCache()
{
	SampledCount = 0;
	bSampled = false;
	SampledCache.Empty();
}

void USH_WaterfallPathComponent::InvalidatePointsCache()
{
	PositionsCache.Empty();
	SmoothedPositionIteration = 0;
	bPositionsCached = false;

	VelocitiesCache.Empty();
	bVelocitiesCached = false;

	NormalsCache.Empty();
	SmoothedNormalIteration = 0;
	bNormalsCached = false;

	TangentsCache.Empty();
	SmoothedTangentIteration = 0;
	bTangentsCached = false;

	DistanceCache.Empty();
	NormalisedDistanceCache.Empty();
	bDistanceCached = false;

	FlowCache.Empty();
	bFlowCached = false;

	TurbulenceCache.Empty();
	bTurbulenceCached = false;

	HitsCache.Empty();
	bHitsCached = false;
}

bool USH_WaterfallPathComponent::CachePath()
{
	if (!SamplePath()) return false;
	if (SampledCache.Num() < 1) return true; //If it's an invalid sample count, return true to avoid endless loops

	bool bReady = true;

	//Cache positions
	if (!bPositionsCached)
	{
		int32 EndIndex = FMath::Min(PositionsCache.Num() + GetWaterfallSettings()->CachingSubsteps, SampledCache.Num() - 1);
		int32 StartIndex = FMath::Min(PositionsCache.Num(), EndIndex);

		for (int i = StartIndex; i <= EndIndex; i++)
		{
			PositionsCache.Add(GetPointPosition(i));
		}

		bPositionsCached = PositionsCache.Num() >= SampledCache.Num();

		if (bPositionsCached)
		{
			//Post-process point positions
			if (GetWaterfallSettings()->PositionSmoothingPerAxis != FVector::ZeroVector)
			{
				while (SmoothedPositionIteration < GetWaterfallSettings()->SmoothingIterations)
				{
					SmoothPositions(GetWaterfallSettings()->PositionSmoothingPerAxis);
					SmoothedPositionIteration++;
				}
			}
		}
		
		bReady = false;
	}

	if (!bVelocitiesCached)
	{
		//Make sure to work with the caching substeps value so that it spaces out calcualtions over multiple ticks - this prevents the editor from freezing
		int32 EndIndex = FMath::Min(VelocitiesCache.Num() + GetWaterfallSettings()->CachingSubsteps, SampledCache.Num() - 1);
		int32 StartIndex = FMath::Min(VelocitiesCache.Num(), EndIndex);

		for (int i = StartIndex; i <= EndIndex; i++)
		{
			VelocitiesCache.Add(GetPointVelocity(i));
		}

		bVelocitiesCached = VelocitiesCache.Num() >= SampledCache.Num();

		bReady = false;
	}

	if (!bNormalsCached)
	{
		//Make sure to work with the caching substeps value so that it spaces out calcualtions over multiple ticks - this prevents the editor from freezing
		int32 EndIndex = FMath::Min(NormalsCache.Num() + GetWaterfallSettings()->CachingSubsteps, SampledCache.Num() - 1);
		int32 StartIndex = FMath::Min(NormalsCache.Num(), EndIndex);

		for (int i = StartIndex; i <= EndIndex; i++)
		{
			NormalsCache.Add(GetPointNormal(i));
		}

		bNormalsCached = NormalsCache.Num() >= SampledCache.Num();

		if (bNormalsCached)
		{
			//Post-process normals positions
			if (SmoothedNormalIteration < GetWaterfallSettings()->SmoothNormalsIteration && GetWaterfallSettings()->SmoothNormalsAlpha > 0.f)
			{
				while (SmoothedNormalIteration < GetWaterfallSettings()->SmoothNormalsIteration)
				{
					SmoothNormals(GetWaterfallSettings()->SmoothNormalsAlpha);
					SmoothedNormalIteration++;
				}
			}
		}

		bReady = false;
	}

	if (!bTangentsCached)
	{
		//Make sure to work with the caching substeps value so that it spaces out calcualtions over multiple ticks - this prevents the editor from freezing
		int32 EndIndex = FMath::Min(TangentsCache.Num() + GetWaterfallSettings()->CachingSubsteps, SampledCache.Num() - 1);
		int32 StartIndex = FMath::Min(TangentsCache.Num(), EndIndex);

		for (int i = StartIndex; i <= EndIndex; i++)
		{
			TangentsCache.Add(GetPointTangent(i));
		}

		bTangentsCached = TangentsCache.Num() >= SampledCache.Num();

		if (bTangentsCached)
		{
			//Post-process Tangents positions
			if (SmoothedTangentIteration < GetWaterfallSettings()->SmoothTangentsIteration && GetWaterfallSettings()->SmoothTangentsAlpha > 0.f)
			{
				while (SmoothedTangentIteration < GetWaterfallSettings()->SmoothTangentsIteration)
				{
					SmoothTangents(GetWaterfallSettings()->SmoothTangentsAlpha);
					SmoothedTangentIteration++;
				}
			}
		}

		bReady = false;
	}

	if (!bDistanceCached)
	{
		//Make sure to work with the caching substeps value so that it spaces out calcualtions over multiple ticks - this prevents the editor from freezing
		int32 EndIndex = FMath::Min(DistanceCache.Num() + GetWaterfallSettings()->CachingSubsteps, SampledCache.Num() - 1);
		int32 StartIndex = FMath::Min(DistanceCache.Num(), EndIndex);

		for (int i = StartIndex; i <= EndIndex; i++)
		{
			float Distance;
			float DistanceNormalised;
			GetPointDistance(i, Distance, DistanceNormalised);

			DistanceCache.Add(Distance);
			NormalisedDistanceCache.Add(DistanceNormalised);
		}

		bDistanceCached = DistanceCache.Num() >= SampledCache.Num();

		bReady = false;
	}

	if (!bFlowCached)
	{
		//Make sure to work with the caching substeps value so that it spaces out calcualtions over multiple ticks - this prevents the editor from freezing
		int32 EndIndex = FMath::Min(FlowCache.Num() + GetWaterfallSettings()->CachingSubsteps, SampledCache.Num() - 1);
		int32 StartIndex = FMath::Min(FlowCache.Num(), EndIndex);

		for (int i = StartIndex; i <= EndIndex; i++)
		{
			FlowCache.Add(GetPointFlow(i, 1.f));
		}

		bFlowCached = FlowCache.Num() >= SampledCache.Num();

		bReady = false;
	}

	if (!bTurbulenceCached)
	{
		//Make sure to work with the caching substeps value so that it spaces out calcualtions over multiple ticks - this prevents the editor from freezing
		int32 EndIndex = FMath::Min(TurbulenceCache.Num() + GetWaterfallSettings()->CachingSubsteps, SampledCache.Num() - 1);
		int32 StartIndex = FMath::Min(TurbulenceCache.Num(), EndIndex);

		for (int i = StartIndex; i <= EndIndex; i++)
		{
			TurbulenceCache.Add(GetPointTurbulence(i));
		}

		bTurbulenceCached = TurbulenceCache.Num() >= SampledCache.Num();

		if (bTurbulenceCached) PropagateTurbulence();

		bReady = false;
	}

	if (!bHitsCached)
	{
		//Make sure to work with the caching substeps value so that it spaces out calcualtions over multiple ticks - this prevents the editor from freezing
		int32 EndIndex = FMath::Min(HitsCache.Num() + GetWaterfallSettings()->CachingSubsteps, SampledCache.Num() - 1);
		int32 StartIndex = FMath::Min(HitsCache.Num(), EndIndex);

		for (int i = StartIndex; i <= EndIndex; i++)
		{
			HitsCache.Add(GetPointHit(i));
		}

		bHitsCached = HitsCache.Num() >= SampledCache.Num();

		bReady = false;
	}

	return bReady;
}

bool USH_WaterfallPathComponent::IsPathCacheConsistent()
{
	int32 GoldenNumber = PositionsCache.Num();

	bool bReturnValue = GoldenNumber > 0
		&& GoldenNumber == VelocitiesCache.Num()
		&& GoldenNumber == NormalsCache.Num()
		&& GoldenNumber == TangentsCache.Num()
		&& GoldenNumber == DistanceCache.Num()
		&& GoldenNumber == NormalisedDistanceCache.Num()
		&& GoldenNumber == TurbulenceCache.Num()
		&& GoldenNumber == HitsCache.Num();

	if (!bReturnValue)
	{
		USH_WaterfallTool2Statics::PrintMessageToLog("Path Inconsistent");
	}

	return bReturnValue;
}

template<typename T>
void ProcessArray(int32 NumPoints, TArray<T>& InOutArray)
{
	FInterpCurve<T> Curve = FInterpCurve<T>();
	for (int i = 0; i < InOutArray.Num(); i++)
	{
		Curve.AddPoint(i / (InOutArray.Num() - 1), InOutArray[i]);
	}

	InOutArray.Empty();

	float SegmentLength = 1.f / (NumPoints - 1); //The first is always 0 and the last is the end, so it's -1
	float WorkingTime = 0.f;
	while (WorkingTime < 1.f)
	{
		InOutArray.Add(Curve.Eval(WorkingTime));
		WorkingTime += SegmentLength;
	}
}

bool USH_WaterfallPathComponent::ResampleCache(int32 NumPoints)
{
	//If it already is this many, then simply return
	if (PositionsCache.Num() == NumPoints || NumPoints < 2) return true;

	ProcessArray(NumPoints, PositionsCache);
	ProcessArray(NumPoints, VelocitiesCache);
	ProcessArray(NumPoints, NormalsCache);
	ProcessArray(NumPoints, TangentsCache);
	ProcessArray(NumPoints, DistanceCache);
	ProcessArray(NumPoints, NormalisedDistanceCache);
	ProcessArray(NumPoints, FlowCache);
	ProcessArray(NumPoints, TurbulenceCache);
	ProcessArray(NumPoints, HitsCache);

	return IsPathCacheConsistent();
}

bool USH_WaterfallPathComponent::IsInBounds(FVector Position, float Margin)
{
	if (!HasBeenSimulated()) return false;

	if (!SearchTree.IsValid())
	{
		if (!BuildSearchTree()) return false;
	}

	return UGeometryScriptLibrary_BoxFunctions::GetBoxPointDistance(SearchTree.Bounds, Position) < Margin;
}

void USH_WaterfallPathComponent::AddNewSimulatedPoint(FVector Position, FSH_SimulatedPoint NewPoint)
{
	UpdateSimulatedPointPosition(SimulatedPoints.Num(), Position);
	NewPoint.PointIndex = SimulatedPoints.Num();
	SimulatedPoints.Add(NewPoint);
}

FVector USH_WaterfallPathComponent::GetSimulatedPointPosition(int32 Index)
{
	return GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
}

bool USH_WaterfallPathComponent::DoesSimulatedPointHaveFX(int32 Index)
{
	return (Index >= 0 && Index < SimulatedPoints.Num()) ? SimulatedPoints[Index].bHasFxAttached : false;
}

void USH_WaterfallPathComponent::SetSimulatedPointHasFX(int32 Index, bool bHasFX)
{
	if (Index >= 0 && Index < SimulatedPoints.Num())
	{
		SimulatedPoints[Index].bHasFxAttached = bHasFX;
	}
}

void USH_WaterfallPathComponent::ResamplePath(int32 NumPoints)
{
	//Recalculate everything in the point
	USH_WaterfallTool2Statics::ResampleSpline(this, NumPoints);

	float SegmentLength = 1.f / NumPoints;
	float WorkingTime = 0.f;
	TArray<FSH_SimulatedPoint> SampledPoints = {};
	while (WorkingTime < 1.f)
	{
		//Get the one or two simulated points that this new point falls between and then lerp between the values, the alpha being the percentage between
		//0 to SimulatedPoints.Num() - 1;

		float ApproximateIndex = FMath::GetMappedRangeValueClamped(TRange<float>(0.f, 1.f), TRange<float>(0.f, SimulatedPoints.Num() - 1), WorkingTime);
		float Index = 0.f;
		float Alpha = FMath::Modf(ApproximateIndex, &Index);
		FSH_SimulatedPoint First = SimulatedPoints[Index];
		FSH_SimulatedPoint Second = SimulatedPoints[Index + 1];

		FSH_SimulatedPoint SampledPoint = FSH_SimulatedPoint(
			FMath::Lerp(First.Velocity, Second.Velocity, Alpha),
			First.State,
			FMath::Lerp(First.Distance, Second.Distance, Alpha),
			USH_WaterfallTool2Statics::SlerpNormals(First.HitNormal, Second.HitNormal, Alpha),
			FMath::Lerp(First.HitForce, Second.HitForce, Alpha),
			FMath::Lerp(First.Age, Second.Age, Alpha),
			FMath::Lerp(First.ObstacleDeviation, Second.ObstacleDeviation, Alpha),
			FMath::Lerp(First.FlowDeviation, Second.FlowDeviation, Alpha)
		);
		SampledPoint.PointIndex = SampledPoints.Num();
		SampledPoints.Add(SampledPoint);

		WorkingTime += SegmentLength;
	}
	//Add the last point because the last point doesn't change
	SampledPoints.Add(SimulatedPoints.Last());
	SimulatedPoints.Empty();
	SimulatedPoints = SampledPoints;
}

void USH_WaterfallPathComponent::UpdateSimulatedPointPosition(int32 Index, FVector NewPosition)
{
	if (GetNumberOfSplinePoints() < Index)
	{
		AddSplinePoint(NewPosition, ESplineCoordinateSpace::World);
	}
	else
	{
		SetLocationAtSplinePoint(Index, NewPosition, ESplineCoordinateSpace::World);
	}
}

bool USH_WaterfallPathComponent::BuildSearchTree(int32 Precision)
{
	if (SimulatedPoints.Num() <= 0) return false;

	int32 MaxIndex = -1;
	SearchTree = BuildSearchNode(0, SimulatedPoints.Num() - 1, 0, Precision, MaxIndex);
	return MaxIndex >= 0;
}

FSH_SearchTreeNode USH_WaterfallPathComponent::BuildSearchNode(int32 FirstIndex, int32 LastIndex, int32 Layer, int32 Precision, int32& OutMaxIndex, FSH_SearchTreeNode* ParentNode)
{
	OutMaxIndex++;
	FSH_SearchTreeNode NewNode = FSH_SearchTreeNode(OutMaxIndex, FirstIndex, LastIndex);

	//1. Create a bounding box from the list of simulated points FirstIndex to LastIndex
	if (Layer == 0)
	{
		//We already have the bounds of the whole path
		NewNode.Bounds = SimulationBounds;
	}
	else
	{
		//Initialise with the first point
		FVector MinBounds = GetSimulatedPointPosition(FirstIndex);
		FVector MaxBounds = GetSimulatedPointPosition(FirstIndex);

		for (int i = FirstIndex + 1; i <= LastIndex; i++)
		{
			FVector SimPos = GetSimulatedPointPosition(i);

			MinBounds.X = FMath::Min(MinBounds.X, SimPos.X);
			MinBounds.Y = FMath::Min(MinBounds.Y, SimPos.Y);
			MinBounds.Z = FMath::Min(MinBounds.Z, SimPos.Z);

			MaxBounds.X = FMath::Min(MaxBounds.X, SimPos.X);
			MaxBounds.Y = FMath::Min(MaxBounds.Y, SimPos.Y);
			MaxBounds.Z = FMath::Min(MaxBounds.Z, SimPos.Z);
		}

		NewNode.Bounds = FBox(MinBounds, MaxBounds);
	}

	//2. Next we need to figure out if this node is a leaf or if we need to divide it further
	if (Precision < 1) Precision = 1;
	int32 NumPoints = LastIndex - FirstIndex;

	if (NumPoints <= Precision)
	{
		NewNode.bIsLeaf = true;
	}
	else
	{
		//3. Divide the points into 2 and build a new node with this NewNode as the parent
		int32 Half = NumPoints / 2;

		BuildSearchNode(FirstIndex, FirstIndex + Half, Layer + 1, Precision, OutMaxIndex, &NewNode);
		BuildSearchNode(FirstIndex + Half + 1, LastIndex, Layer + 1, Precision, OutMaxIndex, &NewNode);
	}

	if (ParentNode) ParentNode->ChildNodes.Add(NewNode);

	return NewNode;
}

bool USH_WaterfallPathComponent::FindNearestPoint(FVector Position, FSH_SimulatedPoint& OutPoint)
{
	OutPoint = FSH_SimulatedPoint();

	if (!HasBeenSimulated()) return false;

	if (!SearchTree.IsValid())
	{
		//Try creating the tree - it may not be made yet
		if (!BuildSearchTree()) return false;
	}

	//Need to calculate nearest distance through recursion, so set here and take it in to the first call of the function
	float NearestDistance = TNumericLimits<float>::Max();
	int32 NearestIndex = SearchTree.FindNearestPoint(Position, NearestDistance, this);

	if (IsValidIndex(NearestIndex, SimulatedPoints))
	{
		OutPoint = SimulatedPoints[NearestIndex];
		return true;
	}

	return false;
}

int32 FSH_SearchTreeNode::FindNearestPoint(FVector Position, float& NearestDistance, USH_WaterfallPathComponent* WaterfallComp)
{
	int32 Index = -1;

	//If the node is a leaf, then we can do the correct checks and return the closest index
	if (bIsLeaf)
	{
		//loop through the points and find the closest point
		float ClosestDistSquared = -1.f;

		for (int i = FirstPointIndex; i <= LastPointIndex; i++)
		{
			float DistSquared = FVector::DistSquared(Position, WaterfallComp->GetSimulatedPointPosition(i));
			if (DistSquared < ClosestDistSquared || ClosestDistSquared < 0)
			{
				ClosestDistSquared = DistSquared;
				Index = i;
			}
		}

		//Verify that the index is valid
		if (Index >= 0 && Index < WaterfallComp->GetNumSimulatedPoints())
		{
			//Set the nearest distance
			float ClosestDistanceSqrt = FMath::Sqrt(ClosestDistSquared);
			if (ClosestDistanceSqrt <= NearestDistance)
			{
				NearestDistance = ClosestDistanceSqrt;
			}
		}
		else Index = -1;
	}
	else //we need to search deeper
	{
		//Do a point-box distance check to see which child we should descend into
		TArray<FSH_PointDistanceNode> PointDistNodes = {};

		for (FSH_SearchTreeNode Node : ChildNodes)
		{
			PointDistNodes.Add(FSH_PointDistanceNode(Node, UGeometryScriptLibrary_BoxFunctions::GetBoxPointDistance(Node.Bounds, Position)));
		}

		//Order the children in order
		PointDistNodes.Sort();

		//Then do the check
		for (FSH_PointDistanceNode Node : PointDistNodes)
		{
			if (Node.Distance < NearestDistance)
			{
				Index = Node.Node.FindNearestPoint(Position, NearestDistance, WaterfallComp);
			}
			else //No more nodes in this branch can contain points that are close enough so end here
			{
				break;
			}
		}

		//Once the check is done, if the check returns a nearest distance larger than the a distance in the array, we need to try the next node
	}

	return Index;
}

bool USH_WaterfallPathComponent::SamplePath()
{
	if (bSampled) return true;

	float PathLength = GetSimulationDistance();

	if (PathLength <= 0.f || CalculatedSampleLength <= 0.f) return true; //invalid sample count - return true to let everything continue but no mesh will be generated

	//See if there was a last sampled point or if we want to start a new point
	FSH_WaterfallSample PreviousPoint = FSH_WaterfallSample();
	if (SampledCache.Num() > 0)
	{
		PreviousPoint = SampledCache[SampledCache.Num() - 1];
	}
	else //Set the intial point
	{
		float Calculation = GetWaterfallSettings()->GetSamplePathMinChecked() * SimulatedPoints.Num();
		PreviousPoint = FSH_WaterfallSample(FMath::FloorToInt(Calculation), FMath::Fractional(Calculation));
	}

	int32 StartSearchIndex = PreviousPoint.Index;

	//Make sure to work with the caching substeps value so that it spaces out calcualtions over multiple ticks - this prevents the editor from freezing
	int32 StartIndex = SampledCount;
	int32 LastIndex = FMath::Min(StartIndex + GetWaterfallSettings()->CachingSubsteps, CalculatedSampleCount - 1);
	for (int i = StartIndex; i <= LastIndex; i++)
	{
		//Find the distance between the two points
		FSH_WaterfallSample SampledPoint;
		float Distance = (CalculatedSampleLength * i) + (GetWaterfallSettings()->GetSamplePathMinChecked() * PathLength);
		bool bSegment = FindSampleByDistance(Distance, StartSearchIndex, SampledPoint);

		if (bSegment)
		{
			StartSearchIndex = SampledPoint.Index;

			//Double check the hit points before add the sampled point
			if (bSampleIncludeHits)
			{
				for (int32 Index : FindHitsBetweenPoints(SampleThreshold, PreviousPoint, SampledPoint))
				{
					SampledCache.Add(FSH_WaterfallSample(Index, 0.f));
				}
			}

			SampledCache.Add(SampledPoint);
			PreviousPoint = SampledPoint;
		}
	}

	//Check if the sampling for this path is finished
	SampledCount = LastIndex + 1;

	if (SampledCount == CalculatedSampleCount && CalculatedSampleCount != 0) bSampled = true;

	return bSampled;
}

bool USH_WaterfallPathComponent::FindSampleByDistance(float Distance, int32 StartIndex, FSH_WaterfallSample& OutSample)
{
	if (SimulatedPoints.Num() > 0 && IsValidIndex(StartIndex, SimulatedPoints))
	{
		float ClampedDistance = FMath::Clamp(Distance, SimulatedPoints[StartIndex].Distance, SimulatedPoints[SimulatedPoints.Num() - 1].Distance);
		for (int i = StartIndex; i < SimulatedPoints.Num() - 1; i++)
		{
			float StartDistance = SimulatedPoints[i].Distance;
			float EndDistance = SimulatedPoints[i + 1].Distance;

			if (EndDistance > StartDistance && UKismetMathLibrary::InRange_FloatFloat(ClampedDistance, StartDistance, EndDistance))
			{
				//Then we're on the segment i to i + 1 - find the 0-1 alpha between the points of where the sample is
				OutSample = FSH_WaterfallSample(i, 1.f - ((EndDistance - ClampedDistance) / (EndDistance - StartDistance)));
				return true;
			}
		}

		OutSample = FSH_WaterfallSample(0, 0.f);
		return false;
	}

	OutSample = FSH_WaterfallSample(-1, -1.f);
	return false;
}

TArray<int32> USH_WaterfallPathComponent::FindHitsBetweenPoints(float HitForce, FSH_WaterfallSample Start, FSH_WaterfallSample End)
{
	TArray<int32> Indexes = {};

	float MinDistance = 0.f;
	float MaxDistance = 0.f;

	FSH_SimulatedPoint OutPoint;
	if (GetInterpolatedSimulatedPoint(Start, OutPoint))
	{
		MinDistance = OutPoint.Distance;
	}

	if (GetInterpolatedSimulatedPoint(End, OutPoint))
	{
		MaxDistance = OutPoint.Distance;
	}

	//Find any points that are hit and that match the hit force threshold
	for (int i = FMath::Max(Start.Index, 0); i <= FMath::Min(End.Index + 1, SimulatedPoints.Num() - 1); i++)
	{
		if (SimulatedPoints[i].State == ESH_SimulatedPointState::Hit
			&& SimulatedPoints[i].Distance < MaxDistance
			&& SimulatedPoints[i].Distance > MinDistance
			&& SimulatedPoints[i].HitForce >= HitForce)
		{
			Indexes.Add(i);
		}
	}

	return Indexes;
}

bool USH_WaterfallPathComponent::GetInterpolatedSimulatedPoint(FSH_WaterfallSample Sample, FSH_SimulatedPoint& OutPoint)
{
	//If index is invalid, return
	if (Sample.Index < 0 || Sample.Index >= SimulatedPoints.Num())
	{
		OutPoint = FSH_SimulatedPoint();
		return false;
	}

	//If we don't need to lerp or if the next point isn't valid then return the point
	if (Sample.Alpha <= 0.f || Sample.Index + 1 < 0 || Sample.Index + 1 >= SimulatedPoints.Num())
	{
		OutPoint = SimulatedPoints[Sample.Index];
		return true;
	}

	//If we don't need to lerp with the next point return the next point
	if (Sample.Alpha >= 1.f)
	{
		OutPoint = SimulatedPoints[Sample.Index + 1];
		return true;
	}

	FVector InterpPos = GetSimulatedPointPosition(Sample.Index);
	OutPoint = InterpolateSimulatedPoint(Sample.Index, Sample.Index + 1, Sample.Alpha, InterpPos);
	return true;
}

FSH_SimulatedPoint USH_WaterfallPathComponent::InterpolateSimulatedPoint(int32 IndexA, int32 IndexB, float Alpha, FVector& InterpolatedPosition)
{
	FSH_SimulatedPoint A = SimulatedPoints[IndexA];
	FSH_SimulatedPoint B = SimulatedPoints[IndexB];

	//If both points are hit, we want to lerp the normal, otherwise we only want to get the normal for the singular point that's hit
	FVector HitNormal = (A.State == ESH_SimulatedPointState::Hit && B.State == ESH_SimulatedPointState::Hit)
		? USH_WaterfallTool2Statics::SlerpNormals(A.HitNormal, B.HitNormal, Alpha)
		: (A.State == ESH_SimulatedPointState::Hit) ? A.HitNormal : B.HitNormal;

	//Repeat the same thing for hit force as above with the Hit Normal
	float HitForce = (A.State == ESH_SimulatedPointState::Hit && B.State == ESH_SimulatedPointState::Hit)
		? FMath::Lerp(A.HitForce, B.HitForce, Alpha)
		: (A.State == ESH_SimulatedPointState::Hit) ? A.HitForce : B.HitForce;

	FVector PosA = GetSimulatedPointPosition(IndexA);
	FVector PosB = GetSimulatedPointPosition(IndexB);
	InterpolatedPosition = FMath::Lerp(PosA, PosB, Alpha);

	return FSH_SimulatedPoint(
		FMath::Lerp(A.Velocity, B.Velocity, Alpha),
		(Alpha >= 0.5f) ? B.State : A.State,
		FMath::Lerp(A.Distance, B.Distance, Alpha),
		HitNormal,
		HitForce,
		FMath::Lerp(A.Age, B.Age, Alpha),
		FMath::Lerp(A.ObstacleDeviation, B.ObstacleDeviation, Alpha),
		FMath::Lerp(A.FlowDeviation, B.FlowDeviation, Alpha)
	);
}

FVector USH_WaterfallPathComponent::GetPointPosition(int32 Index)
{
	float NormalOffset = GetWaterfallSettings()->MeshNormalOffset;

	if (GetWaterfallSettings()->MeshNormalOffsetCurve)
	{
		NormalOffset *= GetWaterfallSettings()->MeshNormalOffsetCurve->GetFloatValue(FMath::Lerp(SimulatedPoints[SampledCache[Index].Index].Distance, SimulatedPoints[FMath::Max(SampledCache[Index].Index + 1, SimulatedPoints.Num() - 1)].Distance, SampledCache[Index].Alpha) / FMath::Max(GetSimulationDistance(), 1.f));
	}

	FVector Normal = GetPointNormal(Index);
	//Override the last normal so that it's not displaced into the water
	if (Index == SampledCache.Num() - 1 && PathTerminatesInWater())
	{
		Normal = Normal.GetSafeNormal(0.001f);
	}

	return FMath::Lerp(
		GetSimulatedPointPosition(SampledCache[Index].Index),
		GetSimulatedPointPosition(FMath::Min(SampledCache[Index].Index + 1, SimulatedPoints.Num() - 1)),
		SampledCache[Index].Alpha)
		+ (NormalOffset * Normal);
}

void USH_WaterfallPathComponent::SmoothPositions(FVector Alpha)
{
	//Find the average of the positions of the sampled points before and after each cached position
	for (int i = 0; i < PositionsCache.Num(); i++)
	{
		if (i + 1 < PositionsCache.Num() && i - 1 >= 0)
		{
			FVector AveragedPosition = (PositionsCache[i] + PositionsCache[i - 1] + PositionsCache[i + 1]) / 3.f;

			//Lerp the XYZ of the position cache with a smaller z to try to keep the projectile path as much as possible
			PositionsCache[i] = FVector(
				FMath::Lerp(PositionsCache[i].X, AveragedPosition.X, Alpha.X),
				FMath::Lerp(PositionsCache[i].Y, AveragedPosition.Y, Alpha.Y),
				FMath::Max(PositionsCache[i].Z, FMath::Lerp(PositionsCache[i].Z, AveragedPosition.Z, Alpha.Z))
			);
		}
	}
}

bool USH_WaterfallPathComponent::PathTerminatesInWater()
{
	if (SimulatedPoints.Num() <= 0) return false;

	return SimulatedPoints[SimulatedPoints.Num() - 1].State == ESH_SimulatedPointState::Killed;
}

float USH_WaterfallPathComponent::GetSimulationWaterLevel()
{
	if (SimulatedPoints.Num() <= 0) return GetComponentLocation().Z;

	return GetSimulatedPointPosition(SimulatedPoints.Num() - 1).Z;
}

FVector USH_WaterfallPathComponent::GetPointNormal(int32 Index)
{
	FVector Normal = FVector::ZeroVector;
	if (Index == 0) Normal = GetUpVector() * -1.f;
	else
	{
		Normal = FVector::CrossProduct(GetPointDirection(Index), GetPointTangent(Index)).GetSafeNormal(0.0001f);
	}

	return GetWaterfallSettings()->bFlipNormals ? Normal * -1.f : Normal;
}

void USH_WaterfallPathComponent::SmoothNormals(float Alpha)
{
	TArray<FVector> UnmodifiedNormals = NormalsCache;

	//Find the average of the normals of the unmodified normals before and after each cached normal
	for (int i = 0; i < UnmodifiedNormals.Num(); i++)
	{
		if (i + 1 < UnmodifiedNormals.Num() && i - 1 >= 0)
		{
			NormalsCache[i] = USH_WaterfallTool2Statics::SlerpNormals(
				UnmodifiedNormals[i],
				USH_WaterfallTool2Statics::SlerpNormals(
					USH_WaterfallTool2Statics::SlerpNormals(
						UnmodifiedNormals[i],
						UnmodifiedNormals[i + 1],
						0.5f),
					USH_WaterfallTool2Statics::SlerpNormals(
						UnmodifiedNormals[i],
						UnmodifiedNormals[i - 1],
						0.5f),
					0.5f),
				Alpha);
		}
	}
}

FVector USH_WaterfallPathComponent::GetPointDirection(int32 Index)
{
	FVector LengthVector = FMath::Lerp(SimulatedPoints[SampledCache[Index].Index].Velocity, SimulatedPoints[FMath::Min(SampledCache[Index].Index + 1, SimulatedPoints.Num() - 1)].Velocity, SampledCache[Index].Alpha);
	float Length = LengthVector.Size();

	if (Length > 0.0001f) return LengthVector / Length;

	//The code should really never reach here but this is just in case the velocity is 0
	if (Index + 1 >= 0 && Index + 1 < SampledCache.Num())
	{
		FVector IndexPosition = FMath::Lerp(GetSimulatedPointPosition(SampledCache[Index].Index), GetSimulatedPointPosition(FMath::Min(SampledCache[Index].Index + 1, SimulatedPoints.Num() - 1)), SampledCache[Index].Alpha);
		FVector NextIndexPosition = FMath::Lerp(GetSimulatedPointPosition(SampledCache[Index + 1].Index), GetSimulatedPointPosition(FMath::Min(SampledCache[Index + 1].Index + 1, SimulatedPoints.Num() - 1)), SampledCache[Index + 1].Alpha);

		return (NextIndexPosition - IndexPosition).GetSafeNormal();
	}

	return FVector(1.f, 0.f, 0.f);
}

FVector USH_WaterfallPathComponent::GetPointTangent(int32 Index)
{
	//Tangent having a 'Z' value can cause issue with some point generation in the first row
	if (Index == 0) return GetRightVector() * -1.f;// *FVector(1.f, 1.f, 0.f);

	FVector Direction = GetPointDirection(Index);
	FVector Tangent = FVector::CrossProduct((Direction.Equals(FVector(0.f, 0.f, 1.f), 0.01f)) ? FVector(1.f, 0.f, 0.f) : FVector(0.f, 0.f, 1.f), Direction).GetSafeNormal(0.0001f);

	//Flatten the last tangent so that it aligns with the water plane correctly
	if (Index == PositionsCache.Num() - 1 && PathTerminatesInWater())
	{
		return Tangent.GetSafeNormal2D(0.0001f);
	}

	return Tangent;
}

void USH_WaterfallPathComponent::SmoothTangents(float Alpha)
{
	TArray<FVector> UnmodifiedTangents = TangentsCache;

	//Find the average of the tangents of the unmodified tangents before and after each cached tangent
	for (int i = 0; i < UnmodifiedTangents.Num(); i++)
	{
		if (i + 1 < UnmodifiedTangents.Num() && i - 1 >= 0)
		{
			TangentsCache[i] = USH_WaterfallTool2Statics::SlerpNormals(
				UnmodifiedTangents[i],
				USH_WaterfallTool2Statics::SlerpNormals(
					USH_WaterfallTool2Statics::SlerpNormals(
						UnmodifiedTangents[i],
						UnmodifiedTangents[i + 1],
						0.5f),
					USH_WaterfallTool2Statics::SlerpNormals(
						UnmodifiedTangents[i],
						UnmodifiedTangents[i - 1],
						0.5f),
					0.5f),
				Alpha);
		}
	}
}

FVector USH_WaterfallPathComponent::GetPointVelocity(int32 Index)
{
	return FMath::Lerp(SimulatedPoints[SampledCache[Index].Index].Velocity, SimulatedPoints[FMath::Min(SampledCache[Index].Index + 1, SimulatedPoints.Num() - 1)].Velocity, SampledCache[Index].Alpha);
}

void USH_WaterfallPathComponent::GetPointDistance(int32 Index, float& Distance, float& DistanceNormalised)
{
	float MaxSimDist = FMath::Max(GetSimulationDistance(), 1.f);

	DistanceNormalised = FMath::GetMappedRangeValueClamped(
		TRange<float>(GetWaterfallSettings()->GetSamplePathMinChecked(), GetWaterfallSettings()->GetSamplePathMaxChecked()),
		TRange<float>(0.f, 1.f),
		FMath::Lerp(SimulatedPoints[SampledCache[Index].Index].Distance, SimulatedPoints[FMath::Min(SampledCache[Index].Index + 1, SimulatedPoints.Num() - 1)].Distance, SampledCache[Index].Alpha) / MaxSimDist
	);

	Distance = DistanceNormalised * MaxSimDist;
}

float USH_WaterfallPathComponent::GetPointFlow(int32 Index, float Min)
{
	float Flow = (1.f / FMath::Max(FMath::Lerp(SimulatedPoints[SampledCache[Index].Index].Velocity, SimulatedPoints[FMath::Min(SampledCache[Index].Index + 1, SimulatedPoints.Num() - 1)].Velocity, SampledCache[Index].Alpha).Size(), Min)) * CalculatedSampleLength;

	return (FlowCache.Num() > 0) ? Flow + FlowCache[FlowCache.Num() - 1] : Flow;
}

float USH_WaterfallPathComponent::GetPointTurbulence(int32 Index)
{
	return GetPointInterpolatedTurbulence(
		SimulatedPoints[SampledCache[Index].Index],
		SimulatedPoints[FMath::Min(SampledCache[Index].Index + 1, SimulatedPoints.Num() - 1)],
		SampledCache[Index].Alpha
	);
}

float USH_WaterfallPathComponent::GetPointInterpolatedTurbulence(FSH_SimulatedPoint A, FSH_SimulatedPoint B, float Alpha)
{
	float HitForceLerp = FMath::Lerp(A.HitForce, B.HitForce, Alpha);
	float BaseTurbulence = ((B.State != ESH_SimulatedPointState::Killed || GetWaterfallSettings()->bTurbulenceIncludesSplash) && HitForceLerp > 0.f) ? FMath::Clamp(HitForceLerp / FMath::Max(GetWaterfallSettings()->TurbulenceRange, 1.f), 0.f, 1.f) : 0.f;

	return BaseTurbulence
		+ FMath::Pow(FMath::Clamp(FMath::Lerp(A.ObstacleDeviation, B.ObstacleDeviation, Alpha) * GetWaterfallSettings()->TurbulenceObstacleWeight, 0.f, 1.f), 2.f)
		+ FMath::Pow(FMath::Clamp(FMath::Lerp(A.FlowDeviation, B.FlowDeviation, Alpha) * GetWaterfallSettings()->TurbulenceFlowWeight, 0.f, 1.f), 2.f);
}

void USH_WaterfallPathComponent::PropagateTurbulence()
{
	if (CalculatedSampleLength > 0.f)
	{
		float UpStreamAlpha = 0.f;
		int32 UpStream = 0;
		float DownStreamAlpha = 0.f;
		int32 DownStream = 0;

		if (GetWaterfallSettings()->TurbulenceSpreadUpStream > 0.f)
		{
			float c = GetWaterfallSettings()->TurbulenceSpreadUpStream / CalculatedSampleLength;
			UpStream = FMath::CeilToInt(c);
			UpStreamAlpha = FMath::Fractional(c); //for last segment
		}

		if (GetWaterfallSettings()->TurbulenceSpreadDownStream > 0.f)
		{
			float c = GetWaterfallSettings()->TurbulenceSpreadDownStream / CalculatedSampleLength;
			DownStream = FMath::CeilToInt(c);
			DownStreamAlpha = FMath::Fractional(c); //for last segment
		}

		if (UpStream > 0 || DownStream > 0)
		{
			for (int i = 0; i < TurbulenceCache.Num(); i++)
			{
				if (TurbulenceCache[i] > 0)
				{
					float BaseTurbulance = TurbulenceCache[i];

					if (UpStream > 0)
					{
						int32 FirstIndex = FMath::Clamp(i - UpStream, 0, TurbulenceCache.Num() - 1);
						int32 LastIndex = FMath::Max(i - 1, 0);

						for (int j = FirstIndex; j <= LastIndex; j++)
						{
							float PropagationAlpha = 1.f - ((i - j) / (LastIndex - FirstIndex + 2));

							//Apply the falloff if needed
							if (GetWaterfallSettings()->TurbulenceFalloff > 0.f) PropagationAlpha = FMath::Pow(PropagationAlpha, GetWaterfallSettings()->TurbulenceFalloff);

							//Lerp the upstream and downstream
							float UpdatedTurbulance = BaseTurbulance;
							if (FirstIndex == j)
							{
								UpdatedTurbulance = BaseTurbulance * PropagationAlpha * UpStreamAlpha;
							}
							else
							{
								UpdatedTurbulance = BaseTurbulance * PropagationAlpha;
							}

							TurbulenceCache[j] = FMath::Clamp(FMath::Max(TurbulenceCache[j], UpdatedTurbulance), 0.f, 1.f);
						}
					}

					if (DownStream > 0)
					{
						int32 FirstIndex = FMath::Min(i + 1, TurbulenceCache.Num() - 1);
						int32 LastIndex = FMath::Clamp(i + DownStream, 0, TurbulenceCache.Num() - 1);

						for (int j = FirstIndex; j <= LastIndex; j++)
						{
							float PropagationAlpha = 1.f - ((j - i) / (LastIndex - FirstIndex + 2));

							//Apply the falloff if needed
							if (GetWaterfallSettings()->TurbulenceFalloff > 0.f) PropagationAlpha = FMath::Pow(PropagationAlpha, GetWaterfallSettings()->TurbulenceFalloff);

							//Lerp the upstream and downstream
							float UpdatedTurbulance = BaseTurbulance;
							if (LastIndex == j)
							{
								UpdatedTurbulance = BaseTurbulance * PropagationAlpha * DownStreamAlpha;
							}
							else
							{
								UpdatedTurbulance = BaseTurbulance * PropagationAlpha;
							}

							TurbulenceCache[j] = FMath::Clamp(FMath::Max(TurbulenceCache[j], UpdatedTurbulance), 0.f, 1.f);
						}
					}
				}
			}
		}
	}
}

float USH_WaterfallPathComponent::GetPointHit(int32 Index)
{
	return FMath::Lerp(SimulatedPoints[SampledCache[Index].Index].HitForce, SimulatedPoints[FMath::Min(SampledCache[Index].Index + 1, SimulatedPoints.Num() - 1)].HitForce, SampledCache[Index].Alpha);
}

#endif //WITH_EDITOR