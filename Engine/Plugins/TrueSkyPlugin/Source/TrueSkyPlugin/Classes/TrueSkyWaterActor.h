// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CoreTypes.h"
#include "GameFramework/Actor.h"
#include "TrueSkyWaterComponent.h"
#include "TrueSkyWaterActor.generated.h"

UCLASS(ClassGroup = Water, hidecategories = (Collision, Replication, Info), showcategories = ("Rendering"), ComponentWrapperClass, ConversionRoot, Blueprintable)
class TRUESKYPLUGIN_API ATrueSkyWater : public AActor
{
	GENERATED_UCLASS_BODY()
	~ATrueSkyWater();

	/** Update and render this water object*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties)
	bool render;

	/** Set how rough the water is via approximate Beaufort value*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "0.0", ClampMax = "12.0"))
	float beaufortScale;

	/** Set the direction the wind is blowing in*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "0.0", ClampMax = "6.28"))
	float windDirection;

	/** Set the water's dependancy on the wind direction*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float windDependency;

	/** Set the Scattering coefficient of the water*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector scattering;

	/** Set the Absorption coefficient of the water*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector absorption;

	/** Set the dimensions of the water object*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "0.0"))
	FVector dimension;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float salinity;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "-20.0", ClampMax = "40.0"))
	float temperature;

	/** Enable advanced water options to allow for finer tuning*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
	bool advancedWaterOptions;

	/** Set the wind speed value*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float windSpeed;

	/** Set the Amplitude of the water waves*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float waveAmplitude;

	/** Set how choppy the water is*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float choppyScale;

private:
	UPROPERTY()
	class UTrueSkyWaterComponent* WaterComponent;

	void PostInitProperties() override;

	void PostLoad() override;

	void PostInitializeComponents() override;

	void Destroyed() override;

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
		
	void TransferProperties();

	FBox GetComponentsBoundingBox(bool bNonColliding) const override;
#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual bool CanEditChange(const UProperty* InProperty)const override;
#endif
	int ID;

	UPROPERTY()
	bool initialised;

public:

	class UTrueSkyWaterComponent* GetWaterComponent() const;

#if WITH_EDITOR
	void EditorApplyTranslation(const FVector & DeltaTranslation,bool bAltDown,bool bShiftDown,bool bCtrlDown) override;
#endif

};