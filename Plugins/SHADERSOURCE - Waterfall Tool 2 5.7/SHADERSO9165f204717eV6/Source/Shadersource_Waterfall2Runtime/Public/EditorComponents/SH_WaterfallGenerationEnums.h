// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.

#pragma once

//#include "EditorComponents/SH_WaterfallGenerationEnums.h"
#include "SH_WaterfallGenerationEnums.generated.h"

/* The different mesh types. */
UENUM(BlueprintType)
enum class ESH_MeshGenerationType : uint8
{
	W2_MG_Singular	UMETA(DisplayName = "Singular"),
	W2_MG_PerPath	UMETA(DisplayName = "Per Path"),
	W2_MG_Cross		UMETA(DisplayName = "Plane"),
	W2_MG_Splash	UMETA(DisplayName = "Splash"),
	W2_MG_All		UMETA(Hidden), //Internal use only
};

namespace StringHelpers
{
	static FString ToString(ESH_MeshGenerationType MeshGenType)
	{
		switch (MeshGenType)
		{
		case ESH_MeshGenerationType::W2_MG_Singular:	return "Singular";
		case ESH_MeshGenerationType::W2_MG_PerPath:		return "PerPath";
		case ESH_MeshGenerationType::W2_MG_Cross:		return "Plane";
		case ESH_MeshGenerationType::W2_MG_Splash:		return "Splash";
		case ESH_MeshGenerationType::W2_MG_All:			return "All";
		}

		return "Invalid";
	}
}

//None of these enums need to be defined at runtime because they're all used for the Ed Mode
#if WITH_EDITORONLY_DATA
/* General Interp Type */
UENUM(BlueprintType)
enum class ESH_W2_Interpolation : uint8
{
	W2_IN_Linear	UMETA(DisplayName = "Linear"),
	W2_IN_Cubic		UMETA(DisplayName = "Cubic"),
};

/* Used in the create mesh buffers functions. */
UENUM(BlueprintType)
enum class ESH_MeshProjectionType : uint8
{
	/* Vertices are nudged away from any impact point. */
	W2_MP_Positive	UMETA(DisplayName = "Positive"),
	/* Vertices are nudged towards any impact point. */
	W2_MP_Negative	UMETA(DisplayName = "Negative"),
	/* Vertices are nudged away from any impact point, and then nudged towards the  impact point. */
	W2_MP_Both		UMETA(DisplayName = "Both"),
};

/* The different available tab states of the Waterfall Tool 2 Editor Mode. */
enum ESH_W2_TabState
{
	//Simple Mode
	W2_TS_Simple,
	//Advanced Mode - Path Generation
	W2_TS_Paths,
	//Advanced Mode - Mesh Generation
	W2_TS_Meshes,
	//Advanced Mode - Bake
	W2_TS_Bake,
	//Advanced Mode - Materials
	W2_TS_Materials,
	//Advanced Mode - FX
	W2_TS_FX,
	//Debug
	W2_TS_Debug,
};

#endif
