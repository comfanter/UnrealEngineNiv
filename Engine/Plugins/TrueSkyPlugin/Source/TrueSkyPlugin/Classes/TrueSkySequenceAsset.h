// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "TrueSkySequenceAsset.generated.h"

UCLASS()
class TRUESKYPLUGIN_API UTrueSkySequenceAsset : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY()
	TArray<uint8> SequenceText;
	void BeginDestroy();
};
