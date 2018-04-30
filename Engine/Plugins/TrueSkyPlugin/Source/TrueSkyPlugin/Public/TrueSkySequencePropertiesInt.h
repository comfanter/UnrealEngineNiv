// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "TrueSkySequencePropertiesInt.generated.h"

// Enumeration for the int-based sky sequencer sky-layer properties.
UENUM( BlueprintType )
enum class ETrueSkySkySequenceLayerPropertiesInt : uint8
{
	TSPROPINT_SkyLayer_NumAltitudes				UMETA( DisplayName = "Altitude Layer Count" ),
	TSPROPINT_SkyLayer_NumElevations			UMETA( DisplayName = "Elevation Layer Count" ),
	TSPROPINT_SkyLayer_NumDistances				UMETA( DisplayName = "Distance Layer Count" ),
	TSPROPINT_SkyLayer_StartDayNumber			UMETA( DisplayName = "Start Day Number (From 1/1/2000)" ),

	TSPROPINT_SkyLayer_Max						UMETA( Hidden )
};

// Enumeration for the int-based sky sequencer sky-keyframe properties.
UENUM( BlueprintType )
enum class ETrueSkySkySequenceKeyframePropertiesInt : uint8
{
	TSPROPINT_SkyKeyframe_StoreAsColours			UMETA( DisplayName = "Store Keyframe as Colours" ),
	TSPROPINT_SkyKeyframe_NumColourAltitudes		UMETA( DisplayName = "Colour Altitude Layer Count" ),
	TSPROPINT_SkyKeyframe_NumColourElevations		UMETA( DisplayName = "Colour Elevation Layer Count" ),
	TSPROPINT_SkyKeyframe_NumColourDistances		UMETA( DisplayName = "Colour Distance Layer Count" ),
	TSPROPINT_SkyKeyframe_AutoMie					UMETA( DisplayName = "Auto-Compute Mie" ),
	TSPROPINT_SkyKeyframe_AutomaticSunPosition		UMETA( DisplayName = "Automatic Sun Position" ),
	TSPROPINT_SkyKeyframe_AutomaticMoonPosition		UMETA( DisplayName = "Automatic Moon Position" ),

	TSPROPINT_SkyKeyframe_Max						UMETA( Hidden )
};

// Enumeration for the int-based sky sequencer cloud-layer properties.
UENUM( BlueprintType )
enum class ETrueSkyCloudSequenceLayerPropertiesInt : uint8
{
	TSPROPINT_CloudLayer_Visible					UMETA( DisplayName = "Enabled" ),
	TSPROPINT_CloudLayer_Wrap						UMETA( DisplayName = "Wrap Horizontally" ),
	TSPROPINT_CloudLayer_OverrideWind				UMETA( DisplayName = "Override Wind" ),
	TSPROPINT_CloudLayer_RenderMode					UMETA( DisplayName = "Render Mode" ),
	TSPROPINT_CloudLayer_MaxPrecipitationParticles	UMETA( DisplayName = "Max Particles (Precipitation)" ),
	TSPROPINT_CloudLayer_DefaultNumSlices			UMETA( DisplayName = "Default Slices" ),
	TSPROPINT_CloudLayer_ShadowTextureSize			UMETA( DisplayName = "Cloud Shadow Texture Size" ),
	TSPROPINT_CloudLayer_GridWidth					UMETA( DisplayName = "Cloud Detail Grid Width" ),
	TSPROPINT_CloudLayer_GridHeight					UMETA( DisplayName = "Cloud Detail Grid Height" ),
	TSPROPINT_CloudLayer_NoiseResolution			UMETA( DisplayName = "3D Noise Resolution" ),
	TSPROPINT_CloudLayer_EdgeNoiseFrequency			UMETA( DisplayName = "Edge Noise Frequency" ),
	TSPROPINT_CloudLayer_EdgeNoiseResolution		UMETA( DisplayName = "Edge Noise Resolution" ),

	TSPROPINT_CloudLayer_Max						UMETA( Hidden )
};

// Enumeration for the float-based sky sequencer cloud-keyframe properties.
UENUM( BlueprintType )
enum class ETrueSkyCloudSequenceKeyframePropertiesInt : uint8
{
	TSPROPINT_CloudKeyframe_Octaves					UMETA( DisplayName = "Noise Generation Octaves" ),
	TSPROPINT_CloudKeyframe_RegionalPrecipitation	UMETA( DisplayName = "Regional Precipitation" ),
	TSPROPINT_CloudKeyframe_LockRainToClouds		UMETA( DisplayName = "Lock Rain to Clouds" ),

	TSPROPINT_CloudKeyframe_Max						UMETA( Hidden )
};
