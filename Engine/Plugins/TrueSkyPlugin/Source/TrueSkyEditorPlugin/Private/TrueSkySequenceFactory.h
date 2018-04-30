// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once
#include "Factories/Factory.h"
#include "TrueSkySequenceFactory.generated.h"

UCLASS()
class UTrueSkySequenceFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	virtual FName GetNewAssetThumbnailOverride() const;
};
