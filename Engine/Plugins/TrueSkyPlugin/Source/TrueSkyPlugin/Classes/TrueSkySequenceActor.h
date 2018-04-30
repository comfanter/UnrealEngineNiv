// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "UObject/ObjectMacros.h"
#include "Engine/Texture2D.h"
#include "Engine/Light.h"
#include "Math/Vector.h"
#include "Math/TransformNonVectorized.h"

#include "ITrueskyPlugin.h"
#include "TrueSkySequencePropertiesFloat.h"
#include "TrueSkySequencePropertiesInt.h"
#include "TrueSkyPropertiesFloat.h"
#include "TrueSkyPropertiesInt.h"

#include "TrueSkySequenceActor.generated.h"

UENUM()
enum class EInterpolationMode : uint8
{
	FixedNumber=0,
	GameTime=1,
	RealTime=2
};

UENUM()
enum class EIntegrationScheme : uint8
{
	Grid=0,
	Fixed=1
};

UENUM()
 enum class PrecipitationOptionsEnum : uint8
 {
     None=0
	 ,VelocityStreaks	=0x1
	 ,SimulationTime	=0x2
	,Paused				=0x4
 };

/*
example
 enum class ETestType : uint8
 {
     None                = 0            UMETA(DisplayName="None"),
     Left                = 0x1        UMETA(DisplayName=" Left"),
     Right                = 0x2        UMETA(DisplayName=" Right"),
 };
 
 ENUM_CLASS_FLAGS(ETestType);

  UPROPERTY()
 TEnumAsByte<EnumName> VarName;
*/
class UTrueSkySequenceAsset;
class ALight;
class UDirectionalLightComponent;
//hideCategories=(Actor, Advanced, Display, Object, Attachment, Movement, Collision, Rendering, Input), 
UCLASS(Blueprintable)
class TRUESKYPLUGIN_API ATrueSkySequenceActor : public AActor
{
	GENERATED_UCLASS_BODY()
	~ATrueSkySequenceActor();
	
	/** The method to use for interpolation. Used from trueSKY 4.1 onwards.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Interpolation)
	EInterpolationMode InterpolationMode;
	
	/** The time for real time interpolation in seconds. Used from trueSKY 4.1 onwards.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Interpolation, meta = ( EditCondition = "bInterpolationRealTime" ) )
	float InterpolationTimeSeconds;

	/** The time for game time interpolation in hours. Used from trueSKY 4.1 onwards.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interpolation, meta = ( EditCondition = "bInterpolationGameTime" ) )
	float InterpolationTimeHours;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Precipitation)
	bool VelocityStreaks;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Precipitation)
	bool SimulationTime;
	
	/** Draw no rain nearer than this distance in Unreal units.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Precipitation)
	float RainNearThreshold;

	/** Assign a directional light to this to update it in real time.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Lighting)
	ALight *Light;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Lighting)
	bool DriveLightDirection;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Lighting,meta=(UIMin = "0.0", UIMax = "1.0"))
	float SunMultiplier;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Lighting,meta=(UIMin = "0.0", UIMax = "1.0"))
	float MoonMultiplier;

	/** Enable water rendering*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water)
	bool RenderWater;

	/** Enable the boundless ocean*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water)
	bool EnableBoundlessOcean;

	/** Set how rough the ocean water is via approximate Beaufort value*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "12.0"))
	float OceanBeaufortScale;

	/** Set the direction the wind is blowing in*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "6.0"))
	float OceanWindDirection;

	/** Set the water's dependency on the wind direction*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OceanWindDependency;

	/** Set the Scattering coefficient of the water*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector OceanScattering;

	/** Set the Absorption coefficient of the water*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector OceanAbsorption;

	/** Enable advanced water options to allow for finer tuning*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water)
	bool AdvancedWaterOptions;

	/** Set the wind speed value*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float OceanWindSpeed;
	
	/** Set the Amplitude of the ocean waves*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OceanWaveAmplitude;

	/** Set how choppy the water is*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float OceanChoppyScale;

	/** Set the level at which foam will start to be generated*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float OceanFoamHeight;

	/** Set the level at which foam will be generated wtih a constant factor*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float OceanFoamChurn;

	/** For debugging only, otherwise leave as -1.0*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSky)
	float FixedDeltaTime;

	/** Returns the spectral radiance as a multiplier for colour values output by trueSKY.
		For example, a pixel value of 1.0, with a reference radiance of 2.0, would represent 2 watts per square metre per steradian. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static float GetReferenceSpectralRadiance();

	UFUNCTION(BlueprintCallable,Category = TrueSky)
	static int32 TriggerAction(FString name);

	UFUNCTION(BlueprintCallable,Category = TrueSky)
	static int32 GetLightning(FVector &start,FVector &end,float &magnitude,FVector &colour);

	UFUNCTION( BlueprintCallable,Category = TrueSky)
	static int32 SpawnLightning(FVector pos1,FVector pos2,float magnitude,FVector colour);

	void Destroyed() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	/** Free all allocated views in trueSKY. This frees up GPU memory.	*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void FreeViews();
	
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static FString GetProfilingText(int32 cpu_level,int32 gpu_level);
	
	/** Returns a string from trueSKY. Valid input is "memory" or "profiling".*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static FString GetText(FString name);

	/** Sets the time value for trueSKY. By default, 0=midnight, 0.5=midday, 1.0= the following midnight, etc.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetTime( float value );
	
	/** Returns the time value for trueSKY. By default, 0=midnight, 0.5=midday, 1.0= the following midnight, etc.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static float GetTime();

	/** Returns the named floating-point property.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static float GetFloat(FString name );
	
	/** Returns the named floating-point property.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	float GetFloatAtPosition(FString name,FVector pos);

	/** Set the named floating-point property.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetFloat(FString name, float value );
	
	/** Returns the named integer property.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetInt(FString name ) ;
	
	/** Set the named integer property.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetInt(FString name, int32 value);
	
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetBool(FString name, bool value );

	UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void SetInteger( ETrueSkyPropertiesInt Property, int32 Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static int32 GetInteger( ETrueSkyPropertiesInt Property );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void GetIntegerRange( ETrueSkyPropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut );

	UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void SetFloatProperty( ETrueSkyPropertiesFloat Property, float Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static float GetFloatProperty( ETrueSkyPropertiesFloat Property );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void GetFloatPropertyRange( ETrueSkyPropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut );

	UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void SetVectorProperty( ETrueSkyPropertiesVector Property, FVector Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static FVector GetVectorProperty( ETrueSkyPropertiesVector Property );

	UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static float GetFloatPropertyAtPosition( ETrueSkyPositionalPropertiesFloat Property,FVector pos,int32 queryId);

	/** Returns the named keyframe property for the keyframe identified.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static float GetKeyframeFloat(int32 keyframeUid,FString name );
	
	/** Set the named keyframe property for the keyframe identified.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetKeyframeFloat(int32 keyframeUid,FString name, float value );
	
	/** Returns the named integer keyframe property for the keyframe identified.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetKeyframeInt(int32 keyframeUid,FString name );
	
	/** Set the named integer keyframe property for the keyframe identified.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetKeyframeInt(int32 keyframeUid,FString name, int32 value );
	
	/** Returns the calculated sun direction.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static FRotator GetSunRotation();

	/** Returns the calculated moon direction.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static FRotator GetMoonRotation();
	
	/** Override the sun direction.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetSunRotation(FRotator r);
	
	/** Override the moon direction.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetMoonRotation(FRotator r);
	
	/** Returns the amount of cloud at the given position.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static float CloudLineTest(int32 queryId,FVector StartPos,FVector EndPos);

	/** Returns the amount of cloud at the given position.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static float CloudPointTest(int32 QueryId,FVector pos);

	/** Returns the cloud shadow at the given position, from 0 to 1.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static float GetCloudShadowAtPosition(int32 QueryId,FVector pos);

	/** Returns the rainfall at the given position, from 0 to 1.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static float GetRainAtPosition(int32 QueryId,FVector pos);

	/** Returns the calculated sun colour in irradiance units, divided by intensity.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static FLinearColor GetSunColor();
	
	/** Returns the Sequence Actor singleton.*/
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=TrueSky)
	static float GetMetresPerUnit();

	/** Returns the calculated moon intensity in irradiance units.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static float GetSunIntensity();
	
	/** Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight actor (sunset occurs later at higher altitudes).
		Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static bool UpdateSunlight(class ALight *DirectionalLight,float multiplier=1.0f,bool include_shadow=false,bool apply_rotation=true);
	
	/** Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight.
	Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static bool UpdateMoonlight(class ALight *DirectionalLight,float multiplier=1.0f,bool include_shadow=false,bool apply_rotation=true);
	
	/** Update a single light with either sunlight or moonlight.
	Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight.
	Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	UFUNCTION(BlueprintCallable, Category = TrueSky)
	static bool UpdateLight(AActor *DirectionalLight, float multiplier=1.0f, bool include_shadow=false, bool apply_rotation=true);

	/** Update a single light with either sunlight or moonlight.
	Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight.
	Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	UFUNCTION(BlueprintCallable, Category = TrueSky)
	static bool UpdateLight2(AActor *DirectionalLight, float sun_multiplier=1.0f, float moon_multiplier=1.0f, bool include_shadow=false, bool apply_rotation=true);

	/** Update a single light with either sunlight or moonlight.
	Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight.
	Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	UFUNCTION(BlueprintCallable, Category = TrueSky)
	static bool UpdateLightComponent(ULightComponent *dlc, float multiplier=1.0f, bool include_shadow=false, bool apply_rotation=true);

	/** Update a single light with either sunlight or moonlight.
	Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight.
	Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	UFUNCTION(BlueprintCallable, Category = TrueSky)
	static bool UpdateLightComponent2(ULightComponent *dlc, float sun_multiplier=1.0f, float moon_multiplier=1.0f, bool include_shadow=false, bool apply_rotation=true);

	/** Render the full sky cubemap to the specified cubemap render target.*/
	UFUNCTION(BlueprintCallable,Category = TrueSky)
	static void RenderToCubemap(class UTrueSkyLightComponent *TrueSkyLightComponent,class UTextureRenderTargetCube *RenderTargetCube);

	/** The maximum sun brightness to be rendered by trueSKY, in radiance units.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSkyRendering)
	float MaxSunRadiance;

	/** Whether to adjust the sun radius with maximum radiance to keep total irradiance constant.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSkyRendering)
	bool AdjustSunRadius;
	
	/** Returns the calculated moon intensity in irradiance units.*/
	UFUNCTION(BlueprintCallable, BlueprintPure,Category=TrueSky)
	static float GetMoonIntensity();

	/** Returns the calculated moon colour in irradiance units, divided by intensity.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=TrueSky)
	static FLinearColor GetMoonColor();

	/** Illuminate the clouds with the specified values from position pos.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetPointLightSource(int32 id,FLinearColor lightColour,float Intensity,FVector pos,float minRadius,float maxRadius);
	
	/** Illuminate the clouds with the specified point light.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetPointLight(APointLight *source);

	/** When multiple trueSKY Sequence Actors are present in a level, the active one is selected using this checkbox.*/
	UPROPERTY(EditAnywhere,Category = TrueSky)
	bool ActiveInEditor;

	/** What is the current active sequence for this actor? */
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSky)
	UTrueSkySequenceAsset* ActiveSequence;
	
	/** Get an identifier for the next sky keyframe that can be altered without requiring a recalculation of the tables.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetNextModifiableSkyKeyframe();
	
	/** Get an identifier for the next cloud keyframe that can be altered without requiring a recalculation of the 3D textures.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetNextModifiableCloudKeyframe(int32 layer);

	/** Get an identifier for the cloud keyframe at the specified index. Returns zero if there is none at that index (e.g. you have gone past the end of the list).*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetCloudKeyframeByIndex(int32 layer,int32 index);

	/** Get an identifier for the next cloud keyframe at or after the specified time.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetNextCloudKeyframeAfterTime(int32 layer,float t);

	/** Get an identifier for the last cloud keyframe before the specified time.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetPreviousCloudKeyframeBeforeTime(int32 layer,float t);
	
	/** Get an identifier for the sky keyframe at the specified index. Returns zero if there is none at that index (e.g. you have gone past the end of the list).*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetSkyKeyframeByIndex(int32 index);

	/** Get an identifier for the next cloud keyframe at or after the specified time.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetNextSkyKeyframeAfterTime(float t);

	/** Get an identifier for the last sky keyframe before the specified time.*/
	UFUNCTION(BlueprintCallable, Category=TrueSky)
	static int32 GetPreviousSkyKeyframeBeforeTime(float t);

	/** Get an indentifier for the interpolated cloud keyframe */
	UFUNCTION(BlueprintCallable, Category = TrueSky)
	static int32 GetInterpolatedCloudKeyframe(int32 layer);

	/** Get an indentifier for the interpolated sky keyframe */
	UFUNCTION(BlueprintCallable, Category = TrueSky)
	static int32 GetInterpolatedSkyKeyframe();
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void SetSkyLayerInt( ETrueSkySkySequenceLayerPropertiesInt Property, int32 Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void SetSkyKeyframeInt( ETrueSkySkySequenceKeyframePropertiesInt Property, int32 KeyframeUID, int32 Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void SetCloudLayerInt( ETrueSkyCloudSequenceLayerPropertiesInt Property, int32 Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void SetCloudKeyframeInt( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32 KeyframeUID, int32 Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static int32 GetSkyLayerInt( ETrueSkySkySequenceLayerPropertiesInt Property );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static int32 GetSkyKeyframeInt( ETrueSkySkySequenceKeyframePropertiesInt Property, int32 KeyframeUID );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static int32 GetCloudLayerInt( ETrueSkyCloudSequenceLayerPropertiesInt Property );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static int32 GetCloudKeyframeInt( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32 KeyframeUID );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void GetSkyLayerIntRange( ETrueSkySkySequenceLayerPropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void GetSkyKeyframeIntRange( ETrueSkySkySequenceKeyframePropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void GetCloudLayerIntRange( ETrueSkyCloudSequenceLayerPropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void GetCloudKeyframeIntRange( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32& ValueMinOut, int32& ValueMaxOut );

	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void SetSkyLayerFloat( ETrueSkySkySequenceLayerPropertiesFloat Property, float Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void SetSkyKeyframeFloat( ETrueSkySkySequenceKeyframePropertiesFloat Property, int32 KeyframeUID, float Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void SetCloudLayerFloat( ETrueSkyCloudSequenceLayerPropertiesFloat Property, float Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void SetCloudKeyframeFloat( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, int32 KeyframeUID, float Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static float GetSkyLayerFloat( ETrueSkySkySequenceLayerPropertiesFloat Property );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static float GetSkyKeyframeFloat( ETrueSkySkySequenceKeyframePropertiesFloat Property, int32 KeyframeUID );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static float GetCloudLayerFloat( ETrueSkyCloudSequenceLayerPropertiesFloat Property );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static float GetCloudKeyframeFloat( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, int32 KeyframeUID );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void GetSkyLayerFloatRange( ETrueSkySkySequenceLayerPropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void GetSkyKeyframeFloatRange( ETrueSkySkySequenceKeyframePropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void GetCloudLayerFloatRange( ETrueSkyCloudSequenceLayerPropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sequence")
	static void GetCloudKeyframeFloatRange( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, float& ValueMinOut, float& ValueMaxOut );

	//UPROPERTY(EditAnywhere, Category=TrueSky)
	//UTextureRenderTarget2D* CloudShadowRenderTarget;
	/** The texture to draw for the moon.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSkyTextures)
	UTexture2D* MoonTexture;
	
	/** The texture to draw for the cosmic background - e.g. the Milky Way.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = TrueSkyTextures)
	UTexture2D* CosmicBackgroundTexture;

	/** If set, trueSKY render rain and snow particles to this target - use it for compositing with the.*/
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSky)
	//UTextureRenderTarget2D* RainOverlayRT;

	/** If set, trueSKY will use this cubemap to light the rain, otherwise a TrueSkyLight will be used.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSkyTextures)
	UTexture* RainCubemap;

	/** Assign a SceneCapture2D actor to mask rain from covered areas.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Precipitation)
	class ASceneCapture2D * RainMaskSceneCapture2D;

	/** Width in Unreal units of the rain mask.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Precipitation)
	float RainMaskWidth;

	/** Height in Unreal units of the rain mask volume.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Precipitation)
	float RainDepthExtent;

	/** The render texture to fill in with atmospheric inscatter values.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = TrueSkyTextures)
	UTextureRenderTarget2D* InscatterRT;

	/** The render texture to fill in with atmospheric loss values.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = TrueSkyTextures)
	UTextureRenderTarget2D* LossRT;
	
	/** The render texture to fill in with the cloud shadow.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = TrueSkyTextures, meta=(EditCondition="bSimulVersion4_1"))
	UTextureRenderTarget2D* CloudShadowRT;

	/** The render texture to fill in with atmospheric loss values.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = TrueSkyTextures)
	UTextureRenderTarget2D* CloudVisibilityRT;

	/** Raytracing integration scheme. Grid is more accurate, but slower.*/
	UPROPERTY(Config=Engine,EditAnywhere,BlueprintReadWrite, Category=TrueSky,meta=(EditCondition="bSimulVersion4_1"))
	EIntegrationScheme IntegrationScheme;

	/** A multiplier for brightness of the trueSKY environment, 1.0 by default.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSkyRendering,meta=(ClampMin = "0.1", ClampMax = "10.0"))
	float Brightness;
	
	/** Tells trueSKY how many metres there are in a single UE4 distance unit. Typically 0.01 (1cm per unit).*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSky,meta=(ClampMin = "0.001", ClampMax = "1000.0"))
	float MetresPerUnit;
	
	/** Tells trueSKY whether and to what extent to apply cloud shadows to the scene in post-processing.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSkyRendering,meta=(EditCondition="!bSimulVersion4_1",ClampMin = "0.0", ClampMax = "1.0"))
	float SimpleCloudShadowing;
	
	/** Tells trueSKY how sharp to make the shadow boundaries.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSkyRendering,meta=(EditCondition="!bSimulVersion4_1",ClampMin = "0.0", ClampMax = "1.0"))
	float SimpleCloudShadowSharpness;

	/** A heuristic distance to discard near depths from depth interpolation, improving accuracy of upscaling.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSky, meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float CloudThresholdDistanceKm;
	
	/** Deprecated. Instead, use MaximumResolution. Tells trueSKY how much to downscale resolution for cloud rendering. The scaling is 2^downscaleFactor.*/
	UPROPERTY(Config=Engine,EditAnywhere,BlueprintReadWrite, Category=TrueSkyRendering,meta=(EditCondition="!bSimulVersion4_1",ClampMin = "1", ClampMax = "4"))
	int32 DownscaleFactor;

	/** The largest cubemap resolution to be used for cloud rendering. Typically 1/4 of the screen width, larger values are more expensive in time and memory.*/
	UPROPERTY(Config=Engine,EditAnywhere,BlueprintReadWrite, Category=TrueSkyRendering, meta=(EditCondition="bSimulVersion4_1",ClampMin = "16", ClampMax = "4096"))
	int32 MaximumResolution;

	/** The size, in pixels, of the sampling area in the full-resolution depth buffer, which is used to find near/far depths.*/
	UPROPERTY(Config=Engine,EditAnywhere,BlueprintReadWrite, Category=TrueSkyRendering, meta=(EditCondition="bSimulVersion4_1",ClampMin = "0.0", ClampMax = "4.0"))
	float DepthSamplingPixelRange;

	/** Tells trueSKY how to spread the cost of rendering over frames. For 1, all pixels are drawn every frame, for amortization 2, it's 2x2, etc.*/
	UPROPERTY(Config=Engine,EditAnywhere,BlueprintReadWrite, Category=TrueSkyRendering,meta=(ClampMin = "1", ClampMax = "8"))
	int32 Amortization;
	
	/** Tells trueSKY how to spread the cost of rendering atmospherics over frames.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSkyRendering, meta = (ClampMin = "1", ClampMax = "8"))
	int32 AtmosphericsAmortization;
	
	/** Grid size for crepuscular rays.*/
	UPROPERTY(EditAnywhere, Category = TrueSkyRendering, meta = (EditCondition="bSimulVersion4_1",ClampMin = "8", ClampMax = "512"))
	int32 GodraysGrid[3];

	/** Tells trueSKY whether to blend clouds with scenery, or to draw them in front/behind depending on altitude.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSky)
	bool DepthBlending;

	/** The smallest size stars can be drawn. If zero they are drawn as points, otherwise as quads. Use this to compensate for artifacts caused by antialiasing.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSky, meta=(EditCondition="bSimulVersion4_1"))
	float MinimumStarPixelSize;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSky)
	bool ShareBuffersForVR;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=TrueSky)
	bool Visible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound)
	class USoundBase* RainSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound)
	 TArray <  USoundBase *> ThunderSounds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound)
	class USoundAttenuation* ThunderAttenuation;
private:
	/** The sound to play when it is raining.*/
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	//class UAudioComponent* AudioComponent;

public:
	// Map indices for the various properties.
	static int32 TSSkyLayerPropertiesFloatIdx;
	static int32 TSSkyKeyframePropertiesFloatIdx;
	static int32 TSCloudLayerPropertiesFloatIdx;
	static int32 TSCloudKeyframePropertiesFloatIdx;
	static int32 TSSkyLayerPropertiesIntIdx;
	static int32 TSSkyKeyframePropertiesIntIdx;
	static int32 TSCloudLayerPropertiesIntIdx;	
	static int32 TSCloudKeyframePropertiesIntIdx;

	static int32 TSPropertiesFloatIdx;
	static int32 TSPropertiesIntIdx;
	static int32 TSPositionalPropertiesFloatIdx;
	static int32 TSPropertiesVectorIdx;
public:
	/*
	 *	Data Structures for trueSKY Property Data.
	*/
	struct TrueSkyPropertyFloatData
	{
		TrueSkyPropertyFloatData( FName Name, float Min, float Max, int64 en) : PropertyName( Name ), ValueMin( Min ), ValueMax( Max ),TrueSkyEnum(en)
		{	}

		FName PropertyName;
		float ValueMin;
		float ValueMax;
		int64 TrueSkyEnum;
	};
	struct TrueSkyPropertyIntData
	{
		TrueSkyPropertyIntData( FName Name, int32 Min, int32 Max, int64 en ) : PropertyName( Name ), ValueMin( Min ), ValueMax( Max ),TrueSkyEnum(en)
		{	}

		FName PropertyName;
		int32 ValueMin;
		int32 ValueMax;
		int64 TrueSkyEnum;
	};

	struct TrueSkyPropertyVectorData
	{
		TrueSkyPropertyVectorData( FName Name, int64 en) : PropertyName( Name ), TrueSkyEnum(en)
		{	}

		FName PropertyName;
		int64 TrueSkyEnum;
	};

	// Enumeration names.
	typedef TMap< int64, TrueSkyPropertyFloatData > TrueSkyPropertyFloatMap;
	static TArray< TrueSkyPropertyFloatMap > TrueSkyPropertyFloatMaps;
	typedef TMap< int64, TrueSkyPropertyIntData > TrueSkyPropertyIntMap;
	static TArray< TrueSkyPropertyIntMap > TrueSkyPropertyIntMaps;
	typedef TMap< int64, TrueSkyPropertyVectorData > TrueSkyPropertyVectorMap;
	static TArray< TrueSkyPropertyVectorMap > TrueSkyPropertyVectorMaps;
protected:
	// Property setup methods.
	void PopulatePropertyFloatMap( UEnum* EnumerationType, TrueSkyPropertyFloatMap& EnumerationMap,FString prefix );
	void PopulatePropertyIntMap( UEnum* EnumerationType, TrueSkyPropertyIntMap& EnumerationMap,FString prefix );
	void PopulatePropertyVectorMap( UEnum* EnumerationType, TrueSkyPropertyVectorMap& EnumerationMap,FString prefix );
	void InitializeTrueSkyPropertyMap( );

	inline void SetFloatPropertyData( int32 EnumType, int64 EnumProperty, float PropertyMin, float PropertyMax )
	{
		TrueSkyPropertyFloatData* data = TrueSkyPropertyFloatMaps[EnumType].Find( EnumProperty );
		if(data)
		{
			data->ValueMin = PropertyMin;
			data->ValueMax = PropertyMax;
		}
	}

	inline void SetIntPropertyData( int32 EnumType, int64 EnumProperty, int32 PropertyMin, int32 PropertyMax )
	{
		TrueSkyPropertyIntData* data = TrueSkyPropertyIntMaps[EnumType].Find( EnumProperty );
		if(data)
		{
			data->ValueMin = PropertyMin;
			data->ValueMax = PropertyMax;
		}
	}
	// Caching modules
	static ITrueSkyPlugin *CachedTrueSkyPlugin;

public:
	static ITrueSkyPlugin *GetTrueSkyPlugin();

public:
	static float Time;

	void PlayThunder(FVector pos);

public:
	void CleanQueries();
	void PostRegisterAllComponents() override;
	void PostInitProperties() override;
	void PostLoad() override;
	void PostInitializeComponents() override;
	bool ShouldTickIfViewportsOnly() const override 
	{
		return true;
	}
	void TickActor( float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction ) override;
#if WITH_EDITOR
	virtual bool CanEditChange(const UProperty* InProperty)const override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void EditorApplyTranslation(const FVector & DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
#endif
	 FBox GetComponentsBoundingBox(bool bNonColliding = false) const override;
protected:
	bool IsActiveActor();
	void TransferProperties();
	void UpdateVolumes();

	UMaterialParameterCollection *TrueSkyMaterialParameterCollection;
	int latest_thunderbolt_id;
	bool bPlaying;
	void FillMaterialParameters();
	static bool bSetTime;

	UPROPERTY( Transient )
	bool bSimulVersion4_1;
	// Properties to hide irrelevant data based on certain settings.
	// Properties to hide irrelevant data based on certain settings.
	UPROPERTY( Transient )
	bool bInterpolationFixed;
	UPROPERTY( Transient )
	bool bInterpolationGameTime;
	UPROPERTY( Transient )
	bool bInterpolationRealTime;
protected:
	FStringAssetReference MoonTextureAssetRef;
	FStringAssetReference MilkyWayAssetRef;
	FStringAssetReference TrueSkyInscatterRTAssetRef;
	FStringAssetReference TrueSkyLossRTAssetRef;
	FStringAssetReference CloudShadowRTAssetRef;
	FStringAssetReference CloudVisibilityRTAssetRef;
public:
	static const FString kDefaultMoon;
	static const FString kDefaultMilkyWay;
	static const FString kDefaultTrueSkyInscatterRT;
	static const FString kDefaultTrueSkyLossRT;
	static const FString kDefaultCloudShadowRT;
	static const FString kDefaultCloudVisibilityRT;

	void SetDefaultTextures();

public:
	bool initializeDefaults;
	float wetness;

public:
	/** Returns AudioComponent subobject **/
	//class UAudioComponent* GetRainAudioComponent() const;
	//class UAudioComponent* GetThunderAudioComponent() const;
	bool IsAnyActorActive() ;
	protected:
	class UTextureRenderTargetCube *CaptureTargetCube;
};
