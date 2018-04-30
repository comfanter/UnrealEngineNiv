// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#pragma once
#include "TrueSkyPluginPrivatePCH.h"
#include "RHIResources.h"
namespace simul
{
	namespace unreal
	{
		struct ActorCrossThreadProperties
		{
			ActorCrossThreadProperties()
				:Destroyed(false)
				,Visible(false)
				,Reset(true)
				,Playing(false)
				,MetresPerUnit(0.01f)
				,Brightness(1.0f)
				,SimpleCloudShadowing(0.0f)
				,SimpleCloudShadowSharpness(0.01f)
				,CloudThresholdDistanceKm(0.1f)
				,DownscaleFactor(2)
				,Amortization(1)
				,AtmosphericsAmortization(4)
				,activeSequence(NULL)
				,MoonTexture(NULL)
				,CosmicBackgroundTexture(NULL)
				,InscatterRT(NULL)
				,LossRT(NULL)
				,CloudVisibilityRT(NULL)
				,CloudShadowRT(nullptr)
				,RainCubemap(NULL)
				,RainDepthRT(nullptr)
				,CaptureCubemapRT(nullptr)
				,Time(0.0f)
				,SetTime(false)
				,RenderWater(true)
				,EnableBoundlessOcean(true)
				,OceanBeaufortScale(3.0)
				,OceanWindDirection(0.0)
				,OceanWindDependancy(0.95)
				,OceanScattering(0.0007, 0.0013, 0.002)
				,OceanAbsorption(0.2916, 0.0474, 0.0092)
				,OceanOffset(0.0,0.0,0.0)
				,AdvancedWaterOptions(false)
				,OceanWindSpeed(30.0)
				,OceanWaveAmplitude(0.35)
				,OceanChoppyScale(2.0)
				,OceanFoamHeight(4.0)
				,OceanFoamChurn(6.0)
				,RainDepthTextureScale(1.0f)
				,RainNearThreshold(30.0f)
				,RainProjMatrix(FMatrix::Identity)
				,DepthBlending(true)
				,MaxSunRadiance(150000.0f)
				,AdjustSunRadius(true)
				,InterpolationMode(0)
				,InterpolationTimeSeconds(10.0f)
				,InterpolationTimeHours(5.0f)
				,MinimumStarPixelSize(2.0f)
				,PrecipitationOptions(0)
				,ShareBuffersForVR(true)
				,MaximumResolution(0)
				,DepthSamplingPixelRange(1.5f)
				,GridRendering(false)
			{
				memset(lightningBolts,0,sizeof(simul::LightningBolt)*4);
				memset(GodraysGrid,0,sizeof(GodraysGrid));
			}
			void Shutdown()
			{
				MoonTexture.SafeRelease();
				CosmicBackgroundTexture.SafeRelease();
				InscatterRT.SafeRelease();
				LossRT.SafeRelease();
				CloudVisibilityRT.SafeRelease();
				CloudShadowRT.SafeRelease();
				RainCubemap.SafeRelease();
				RainDepthRT.SafeRelease();
				CaptureCubemapRT.SafeRelease();
				CaptureCubemapTrueSkyLightComponent.Reset();
			}
			bool Destroyed;
			bool Visible;
			bool Reset;
			bool Playing;
			float MetresPerUnit;
			float Brightness;
			float SimpleCloudShadowing;
			float SimpleCloudShadowSharpness;
			float CloudThresholdDistanceKm;
			int DownscaleFactor;
			int Amortization;
			int AtmosphericsAmortization;
			class UTrueSkySequenceAsset *activeSequence;
			FTextureRHIRef MoonTexture;
			FTextureRHIRef CosmicBackgroundTexture;
			FTextureRHIRef InscatterRT;
			FTextureRHIRef LossRT;
			FTextureRHIRef CloudVisibilityRT;
			FTextureRHIRef CloudShadowRT;
			FTextureRHIRef RainCubemap;
			FTextureRHIRef RainDepthRT;
			FTextureRHIRef CaptureCubemapRT;
			TWeakObjectPtr <UTrueSkyLightComponent> CaptureCubemapTrueSkyLightComponent;
			float Time;
			bool SetTime;
			FTransform Transform;
			bool RenderWater;
			bool EnableBoundlessOcean;
			float OceanBeaufortScale;
			float OceanWindDirection;
			float OceanWindDependancy;
			FVector OceanScattering;
			FVector OceanAbsorption;
			FVector OceanOffset;
			bool AdvancedWaterOptions;
			float OceanWindSpeed;
			float OceanWaveAmplitude;
			float OceanChoppyScale;
			float OceanFoamHeight;
			float OceanFoamChurn;
			FMatrix RainDepthMatrix;
			float RainDepthTextureScale;
			float RainNearThreshold;
			FMatrix RainProjMatrix;
			simul::LightningBolt lightningBolts[4];
			bool DepthBlending;
			float MaxSunRadiance;
			bool AdjustSunRadius;
			int InterpolationMode;
			float InterpolationTimeSeconds;
			float InterpolationTimeHours;
			float MinimumStarPixelSize;
			uint8_t PrecipitationOptions;
			bool ShareBuffersForVR;
			int MaximumResolution;
			float DepthSamplingPixelRange;
			bool GridRendering;
			int32 GodraysGrid[3];
			TArray<TPair<int64,vec3> > SetVectors;
			TArray<TPair<int64,float> > SetFloats;
			TArray<TPair<int64,int32> > SetInts;

			FCriticalSection CriticalSection;
		};
		extern ActorCrossThreadProperties *GetActorCrossThreadProperties();
	}
}
