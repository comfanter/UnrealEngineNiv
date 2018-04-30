// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once
#include "Components/SkyLightComponent.h"
#include "TrueSkyLightComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=Lights,HideCategories=(Trigger,Activation,"Components|Activation",Physics), meta=(BlueprintSpawnableComponent))
class TRUESKYPLUGIN_API UTrueSkyLightComponent : public USkyLightComponent
{
	GENERATED_UCLASS_BODY()

	/** How often to update */
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSky,meta=(ClampMin = "1", ClampMax = "100"))
	int32 UpdateFrequency;
	/** How much to blend new updates with previous values */
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSky,meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float Blend;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSky, meta=(DisplayName = "Diffuse Multiplier", UIMin = "0.0", UIMax = "20.0"))
	float DiffuseMultiplier;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSky, meta=(DisplayName = "Specular Multiplier", UIMin = "0.01", UIMax = "20.0"))
	float SpecularMultiplier;

	FTexture *GetCubemapTexture();

	//~ Begin UObject Interface
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void PostInterpChange(UProperty* PropertyThatChanged) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const UProperty* InProperty) const override;
	virtual void CheckForErrors() override;
#endif // WITH_EDITOR
	virtual void BeginDestroy() override;
	virtual bool IsReadyForFinishDestroy() override;
	//~ End UObject Interface
	virtual FActorComponentInstanceData* GetComponentInstanceData() const override;

	/** 
	 * Recaptures the scene for the skylight. 
	 * This is useful for making sure the sky light is up to date after changing something in the world that it would capture.
	 * Warning: this is very costly and will definitely cause a hitch.
	 */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|SkyLight",meta=(DeprecatedFunction))
	void RecaptureSky();

	static TArray<UTrueSkyLightComponent*> &GetTrueSkyLightComponents()
	{
		return TrueSkyLightComponents;
	}
	FSkyLightSceneProxy *GetSkyLightSceneProxy()
	{
		return SceneProxy;
	}

	void SetHasUpdated();

	void UpdateDiffuse(float *shValues);
	/** 
	* Forces the skylight to regenerate - typically 3 frames of latency, but much quicker than waiting for the usual updates.. 
	* This is useful when time or lighting has changed significantly since the previous frame.
	*/
	UFUNCTION(BlueprintCallable, Category="TrueSky")
	void ResetTrueSkyLight();

	void SetInitialized(bool b)
	{
		bIsInitialized=b;
		bDiffuseInitialized&=b;
	}
	bool IsInitialized() const
	{
		return bIsInitialized;
	}

	static bool AllSkylightsValid;
protected:
	static TArray<UTrueSkyLightComponent*> TrueSkyLightComponents;
	//~ Begin UActorComponent Interface
	virtual void CreateRenderState_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
	//~ Begin UActorComponent Interface

	friend class FSkyLightSceneProxy;
	friend class FTrueSkyPlugin;
	bool bIsInitialized;
	bool bDiffuseInitialized;
	float AverageBrightness;

};



