// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "TrueSkyPropertiesInt.generated.h"

// Enumeration for the int-based sky  properties.
UENUM( BlueprintType )
enum class ETrueSkyPropertiesInt : uint8
{
	TSPROPINT_OnscreenProfiling			UMETA( DisplayName = "Onscreen Profiling" ),

	TSPROPINT_Max				UMETA( Hidden )
};
