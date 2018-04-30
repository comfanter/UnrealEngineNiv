#pragma once
#include "TrueSkyPluginPrivatePCH.h"

struct WaterActorCrossThreadProperties
{
	WaterActorCrossThreadProperties()
		:Render(false)
		,Destroyed(false)
		,Reset(true)
		,BoundedObjectCreated(false)
		,BeaufortScale(3.0)
		,WindDirection(0.0)
		,WindDependency(0.95)
		,Scattering(0.17, 0.2, 0.234)
		,Absorption(0.2916, 0.0474, 0.0092)
		,Location(0.0, 0.0, 0.0)
		,Dimension(50.0,50.0,50.0)
		,Salinity(0.0)
		,Temperature(0.0)
		,AdvancedWaterOptions(false)
		,WindSpeed(30.0)
		,WaveAmplitude(0.5)
		,ChoppyScale(2.0)
		,ID(-1)
	{
	}
		bool Render;
		bool Destroyed;
		bool Reset;
		bool BoundedObjectCreated;
		
		float BeaufortScale;
		float WindDirection;
		float WindDependency;
		FVector Scattering;
		FVector Absorption;
		FVector Location;
		FVector Dimension;
		float Salinity;
		float Temperature;

		bool AdvancedWaterOptions;
		float WindSpeed;
		float WaveAmplitude;
		float ChoppyScale;
		int ID;
};

extern WaterActorCrossThreadProperties* GetWaterActorCrossThreadProperties(int ID);
extern void AddWaterActorCrossThreadProperties(WaterActorCrossThreadProperties* newProperties);
extern void RemoveWaterActorCrossThreadProperties(int ID);