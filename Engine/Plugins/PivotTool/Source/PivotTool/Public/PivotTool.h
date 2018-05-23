// Copyright 2018 BruceLee, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

#define PIVOT_TOOL_DEBUG 0

class FPivotToolModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};