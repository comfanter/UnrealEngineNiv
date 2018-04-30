// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "TrueSkySequencePropertiesFloat.generated.h"

// Enumeration for the float-based sky sequencer sky-layer properties.
UENUM( BlueprintType )
enum class ETrueSkySkySequenceLayerPropertiesFloat : uint8
{
	TSPROPFLT_SkyLayer_BrightnessPower			UMETA( DisplayName = "Brightness Power" ),
	TSPROPFLT_SkyLayer_MaxAltitudeKm			UMETA( DisplayName = "Max Altitude (Km)" ),
	TSPROPFLT_SkyLayer_MaxDistanceKm			UMETA( DisplayName = "Max Distance (Km)" ),
	TSPROPFLT_SkyLayer_OvercastEffectStrength	UMETA( DisplayName = "Overcast Effect Strength" ),
	TSPROPFLT_SkyLayer_OzoneStrength			UMETA( DisplayName = "Ozone Strength" ),
	TSPROPFLT_SkyLayer_Emissivity				UMETA( DisplayName = "Emissivity" ),
	TSPROPFLT_SkyLayer_SunRadiusArcMinutes		UMETA( DisplayName = "Diameter (Sun)" ),
	TSPROPFLT_SkyLayer_SunBrightnessLimit		UMETA( DisplayName = "Brightness Limit (Sun)" ),
	TSPROPFLT_SkyLayer_LatitudeRadians			UMETA( DisplayName = "Latitude" ),
	TSPROPFLT_SkyLayer_LongitudeRadians			UMETA( DisplayName = "Longitutde" ),
	TSPROPFLT_SkyLayer_TimezoneHours			UMETA( DisplayName = "Timezone +/-" ),
	TSPROPFLT_SkyLayer_MoonAlbedo				UMETA( DisplayName = "Albedo (Moon)" ),
	TSPROPFLT_SkyLayer_MoonRadiusArcMinutes		UMETA( DisplayName = "Diameter (Moon)" ),
	TSPROPFLT_SkyLayer_MaxStarMagnitude			UMETA( DisplayName = "Max Magnitude (Star)" ),
	TSPROPFLT_SkyLayer_StarBrightness			UMETA( DisplayName = "Brightness (Stars)" ),
	TSPROPFLT_SkyLayer_BackgroundBrightness		UMETA( DisplayName = "Brightness (Background)" ),

	TSPROPFLT_SkyLayer_Max						UMETA( Hidden )
};

// Enumeration for the float-based sky sequencer sky-keyframe properties.
UENUM( BlueprintType )
enum class ETrueSkySkySequenceKeyframePropertiesFloat : uint8
{
	TSPROPFLT_SkyKeyframe_Daytime					UMETA( DisplayName = "Time" ),
	TSPROPFLT_SkyKeyframe_SeaLevelTemperatureK		UMETA( DisplayName = "Sea Level Deg. C" ),
	TSPROPFLT_SkyKeyframe_Haze						UMETA( DisplayName = "Haze (i.e. Fog)" ),
	TSPROPFLT_SkyKeyframe_HazeBaseKm				UMETA( DisplayName = "Haze Base (Km)" ),
	TSPROPFLT_SkyKeyframe_HazeScaleKm				UMETA( DisplayName = "Haze Scale (Km)" ),
	TSPROPFLT_SkyKeyframe_HazeEccentricity			UMETA( DisplayName = "Haze Eccentricity" ),
	TSPROPFLT_SkyKeyframe_MieRed					UMETA( DisplayName = "Mie Coefficient (Red)" ),
	TSPROPFLT_SkyKeyframe_MieGreen					UMETA( DisplayName = "Mie Coefficient (Green)" ),
	TSPROPFLT_SkyKeyframe_MieBlue					UMETA( DisplayName = "Mie Coefficient (Blue)" ),
	TSPROPFLT_SkyKeyframe_SunElevation				UMETA( DisplayName = "Sun Elevation" ),
	TSPROPFLT_SkyKeyframe_SunAzimuth				UMETA( DisplayName = "Sun Azimuth" ),
	TSPROPFLT_SkyKeyframe_MoonElevation				UMETA( DisplayName = "Moon Elevation" ),
	TSPROPFLT_SkyKeyframe_MoonAzimuth				UMETA( DisplayName = "Moon Azimuth" ),

	TSPROPFLT_SkyKeyframe_Max						UMETA( Hidden )
};

// Enumeration for the float-based sky sequencer cloud-layer properties.
UENUM( BlueprintType )
enum class ETrueSkyCloudSequenceLayerPropertiesFloat : uint8
{
	TSPROPFLT_CloudLayer_PrecipitationThresholdKm	UMETA( DisplayName = "Precipitation Threshold (Km)" ),
	TSPROPFLT_CloudLayer_PrecipitationRadiusMetres	UMETA( DisplayName = "Precipitation Radius (Metres)" ),
	TSPROPFLT_CloudLayer_RainFallSpeedMS			UMETA( DisplayName = "Rain Fall Speed (Meters/Second)" ),
	TSPROPFLT_CloudLayer_RainDropSizeMm				UMETA( DisplayName = "Rain Drop Size (mm)" ),
	TSPROPFLT_CloudLayer_CloudShadowStrength		UMETA( DisplayName = "Cloud Shadow Strength" ),
	TSPROPFLT_CloudLayer_CloudShadowRange			UMETA( DisplayName = "Cloud Shadow Range" ),
	TSPROPFLT_CloudLayer_MaxCloudDistanceKm			UMETA( DisplayName = "Max Cloud Distance (Km)" ),
	TSPROPFLT_CloudLayer_NoisePeriod				UMETA( DisplayName = "Fractal Noise Period" ),
	TSPROPFLT_CloudLayer_EdgeNoisePersistence		UMETA( DisplayName = "Edge Noise Persistance" ),

	TSPROPFLT_CloudLayer_Max						UMETA( Hidden )
};

// Enumeration for the float-based sky sequencer cloud-keyframe properties.
UENUM( BlueprintType )
enum class ETrueSkyCloudSequenceKeyframePropertiesFloat : uint8
{
	TSPROPFLT_CloudKeyframe_Cloudiness					UMETA( DisplayName = "Cloudiness" ),
	TSPROPFLT_CloudKeyframe_CloudBase					UMETA( DisplayName = "Cloud Base" ),
	TSPROPFLT_CloudKeyframe_CloudHeight					UMETA( DisplayName = "Cloud Height" ),
	TSPROPFLT_CloudKeyframe_CloudWidthKm				UMETA( DisplayName = "Cloud Width (Km)" ),
//	TSPROPFLT_CloudKeyframe_CloudWidthMetres			UMETA( DisplayName = "Cloud Width (m)" ),			// Hiding this for simplicity.
	TSPROPFLT_CloudKeyframe_WindSpeed					UMETA( DisplayName = "Wind Speed" ),
	TSPROPFLT_CloudKeyframe_WindHeadingDegrees			UMETA( DisplayName = "Wind Heading Degrees" ),
	TSPROPFLT_CloudKeyframe_DistributionBaseLayer		UMETA( DisplayName = "Distribution Base Layer" ),
	TSPROPFLT_CloudKeyframe_DistributionTransition		UMETA( DisplayName = "Distribution Transition" ),
	TSPROPFLT_CloudKeyframe_UpperDensity				UMETA( DisplayName = "Upper Density" ),
	TSPROPFLT_CloudKeyframe_Persistence					UMETA( DisplayName = "Persistence" ),
	TSPROPFLT_CloudKeyframe_WorleyNoise					UMETA( DisplayName = "Worley Noise" ),
	TSPROPFLT_CloudKeyframe_WorleyScale					UMETA( DisplayName = "Worley Scale" ),
	TSPROPFLT_CloudKeyframe_Diffusivity					UMETA( DisplayName = "Diffusivity" ),
	TSPROPFLT_CloudKeyframe_DirectLight					UMETA( DisplayName = "Direct Light" ),
	TSPROPFLT_CloudKeyframe_LightAsymmetry				UMETA( DisplayName = "Light Asymmetry" ),
	TSPROPFLT_CloudKeyframe_IndirectLight				UMETA( DisplayName = "Indirect Light" ),
	TSPROPFLT_CloudKeyframe_AmbientLight				UMETA( DisplayName = "Ambient Light" ),
	TSPROPFLT_CloudKeyframe_Extinction					UMETA( DisplayName = "Extinction" ),
	TSPROPFLT_CloudKeyframe_GodrayStrength				UMETA( DisplayName = "God Ray Strength" ),
	TSPROPFLT_CloudKeyframe_FractalWavelength			UMETA( DisplayName = "Fractal Wavelength" ),
	TSPROPFLT_CloudKeyframe_FractalAmplitude			UMETA( DisplayName = "Fractal Amplitude" ),
	TSPROPFLT_CloudKeyframe_EdgeWorleyScale				UMETA( DisplayName = "Edge Worley Scale" ),
	TSPROPFLT_CloudKeyframe_EdgeWorleyNoise				UMETA( DisplayName = "Edge Worley Noise" ),
	TSPROPFLT_CloudKeyframe_EdgeSharpness				UMETA( DisplayName = "Edge Sharpness" ),
	TSPROPFLT_CloudKeyframe_BaseNoiseFactor				UMETA( DisplayName = "Base Noise Factor" ),
	TSPROPFLT_CloudKeyframe_Churn						UMETA( DisplayName = "Churn" ),
	TSPROPFLT_CloudKeyframe_RainRadiusKm				UMETA( DisplayName = "Rain Radius (Km)" ),
	TSPROPFLT_CloudKeyframe_Precipitation				UMETA( DisplayName = "Precipitation Strength" ),
	TSPROPFLT_CloudKeyframe_RainEffectStrength			UMETA( DisplayName = "Rain Effect Strength" ),
	TSPROPFLT_CloudKeyframe_RainToSnow					UMETA( DisplayName = "Rain to Snow" ),
	TSPROPFLT_CloudKeyframe_PrecipitationWindEffect		UMETA( DisplayName = "Precipitation Wind Effect" ),
	TSPROPFLT_CloudKeyframe_PrecipitationWaver			UMETA( DisplayName = "Precipitation Waver" ),
	TSPROPFLT_CloudKeyframe_Offsetx						UMETA( DisplayName = "Offset X (Map)" ),
	TSPROPFLT_CloudKeyframe_Offsety						UMETA( DisplayName = "Offset Y (Map)" ),
	TSPROPFLT_CloudKeyframe_RainCentreXKm				UMETA( DisplayName = "Region Centre X (Precipitation)" ),
	TSPROPFLT_CloudKeyframe_RainCentreYKm				UMETA( DisplayName = "Region Centre Y (Precipitation)" ),

	TSPROPFLT_CloudKeyframe_Max							UMETA( Hidden )
};
