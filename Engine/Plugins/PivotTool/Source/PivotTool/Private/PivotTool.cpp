// Copyright 2018 BruceLee, Inc. All Rights Reserved.

#include "PivotTool.h"
#include "PivotToolStyle.h"
#include "PivotToolEdMode.h"

#define LOCTEXT_NAMESPACE "FPivotToolModule"

void FPivotToolModule::StartupModule()
{
	FPivotToolStyle::Initialize();
	FPivotToolStyle::ReloadTextures();
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FEditorModeRegistry::Get().RegisterMode<FPivotToolEdMode>(FPivotToolEdMode::EM_PivotToolEdModeId, LOCTEXT("PivotToolEdModeName", "PivotToolEdMode"), FSlateIcon(FPivotToolStyle::GetStyleSetName(), "PivotTool.PivotToolMode","PivotTool.PivotToolMode.Small"), true);
}

void FPivotToolModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FPivotToolStyle::Shutdown();
	FEditorModeRegistry::Get().UnregisterMode(FPivotToolEdMode::EM_PivotToolEdModeId);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPivotToolModule, PivotTool)