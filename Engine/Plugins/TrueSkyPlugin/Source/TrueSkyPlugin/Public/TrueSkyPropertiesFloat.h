// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "TrueSkyPropertiesFloat.generated.h"

// Enumeration for the float-based truesky properties.
UENUM( BlueprintType )
enum class ETrueSkyPropertiesFloat : uint8
{
	TSPROPFLOAT_DepthSamplingPixelRange		UMETA( DisplayName = "depth sampling pixel range" ),
	TSPROPFLOAT_NearestRainMetres			UMETA( DisplayName = "nearest rain metres" ),
	TSPROPFLOAT_CloudThresholdDistanceKm	UMETA( DisplayName = "Cloud Threshold Distance Km" ),
	TSPROPFLOAT_CloudshadowRangeKm			UMETA( DisplayName = "Cloud Shadowrange km" ),
	TSPROPFLOAT_LatitudeDegrees				UMETA( DisplayName = "latitude degrees" ),
	TSPROPFLOAT_LongitudeDegrees			UMETA( DisplayName = "longitude degrees" ),
	TSPROPFLOAT_MaxSunRadiance				UMETA( DisplayName = "max sun radiance" ),
	TSPROPFLOAT_ReferenceSpectralRadiance	UMETA( DisplayName = "Reference Spectral Radiance" ),
	TSPROPFLOAT_Max							UMETA( Hidden )
};

// Enumeration for the positional float-based truesky properties.
UENUM( BlueprintType )
enum class ETrueSkyPositionalPropertiesFloat : uint8
{
	TSPROPFLOAT_Cloudiness				UMETA( DisplayName = "Cloudiness" ),
	TSPROPFLOAT_Precipitation			UMETA( DisplayName = "Precipitation" ),
	TSPROPFLOAT_Max						UMETA( Hidden )
};

// Enumeration for the positional float-based truesky properties.
UENUM( BlueprintType )
enum class ETrueSkyPropertiesVector : uint8
{
	TSPROPVECTOR_CloudShadowOriginKm		UMETA( DisplayName = "Cloud Shadow Origin Km" ),
	TSPROPVECTOR_CloudShadowScaleKm			UMETA( DisplayName = "Cloud Shadow Scale Km" ),
	TSPROPVECTOR_SunDirection				UMETA( DisplayName = "Override direction of the sun." ),
	TSPROPVECTOR_MoonDirection				UMETA( DisplayName = "Override direction of the moon." ),
	TSPROPVECTOR_Max						UMETA( Hidden )
};