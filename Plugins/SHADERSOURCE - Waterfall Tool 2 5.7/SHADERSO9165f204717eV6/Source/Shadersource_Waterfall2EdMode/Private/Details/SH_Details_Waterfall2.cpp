// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "Details/SH_Details_Waterfall2.h"

void SH_Details_Waterfall2::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.EditCategory(FName("WaterfallSettings"), FText::GetEmpty(), ECategoryPriority::Important);

	//This category is created entirely in a Details Customisation so modifying its Display name here doesn't seem to work
	DetailBuilder.EditCategory("Selected Points", FText::FromString("Top Spline Points"), ECategoryPriority::Important);

	DetailBuilder.EditCategory("Spline", FText::FromString("Top Spline Settings"), ECategoryPriority::Important);
	DetailBuilder.EditCategory("Editor", FText::FromString("Top Spline Editor Settings"), ECategoryPriority::Important);

	DetailBuilder.HideCategory("Actions");
	DetailBuilder.HideCategory("StaticMesh");
	DetailBuilder.HideCategory("Dynamic Mesh Component");
	DetailBuilder.HideCategory("BodySetup");

	DetailBuilder.HideCategory("PathSettings");
	DetailBuilder.HideCategory("MeshGenerationSettings");
	DetailBuilder.HideCategory("BakeMeshSettings");

	DetailBuilder.HideCategory("InitialSpawn");
	DetailBuilder.HideCategory("Simulation");
	DetailBuilder.HideCategory("Speed");
	DetailBuilder.HideCategory("Direction");
	DetailBuilder.HideCategory("Physics");
	DetailBuilder.HideCategory("Avoidance");
	DetailBuilder.HideCategory("Flow");

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
	DetailBuilder.HideCategory("CrossPlaneSettings");
	DetailBuilder.HideCategory("SplashMeshSettings");
	DetailBuilder.HideCategory("FxMeshSettings");
	DetailBuilder.HideCategory("Bulge");

	DetailBuilder.HideCategory("Materials");
	DetailBuilder.HideCategory("FX");
	DetailBuilder.HideCategory("BakeSettings");
	DetailBuilder.HideCategory("Debug");

	DetailBuilder.HideCategory("Waterfall");

	DetailBuilder.HideCategory("Niagara");
	DetailBuilder.HideCategory("UserParameters"); //This category is created entirely in a Details Customisation so hiding it here doesn't seem to work
	DetailBuilder.HideCategory("NiagaraUtilities"); //This category is created entirely in a Details Customisation so hiding it here doesn't seem to work
	DetailBuilder.HideCategory("Attachment");
	DetailBuilder.HideCategory("Randomness");
	DetailBuilder.HideCategory("Parameters");
	DetailBuilder.HideCategory("Warmup");
	DetailBuilder.HideCategory("LOD");
	DetailBuilder.HideCategory("Rendering");
	DetailBuilder.HideCategory("Compilation");
	DetailBuilder.HideCategory("SFX");
	DetailBuilder.HideCategory("VFX Settings");
	DetailBuilder.HideCategory("HLOD");
	DetailBuilder.HideCategory("Lighting");
	DetailBuilder.HideCategory("PathTracing");
	DetailBuilder.HideCategory("TextureStreaming");
	DetailBuilder.HideCategory("Mobile");
	DetailBuilder.HideCategory("Navigation");
	DetailBuilder.HideCategory("RayTracing");
	DetailBuilder.HideCategory("Sound");
	DetailBuilder.HideCategory("Attenuation");
	DetailBuilder.HideCategory("Concurrency");
	DetailBuilder.HideCategory("Subtitles");
	DetailBuilder.HideCategory("Randomization");
	DetailBuilder.HideCategory("Analysis");
	DetailBuilder.HideCategory("MaterialParameters");
	DetailBuilder.HideCategory("VirtualTexture");
	DetailBuilder.HideCategory("Input");

	DetailBuilder.HideCategory("SFX Settings");
	DetailBuilder.HideCategory("Top VFX Settings");
	DetailBuilder.HideCategory("Middle VFX Settings");
	DetailBuilder.HideCategory("Bottom VFX Settings");
	DetailBuilder.HideCategory("FX (Individual Points)");
}