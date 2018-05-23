// Copyright 2018 BruceLee, Inc. All Rights Reserved.

#include "PivotToolEdModeToolkit.h"
#include "PivotToolEdMode.h"
#include "Engine/Selection.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorModeManager.h"
#include "SPivotToolWidget.h"

#define LOCTEXT_NAMESPACE "FPivotToolEdModeToolkit"

FPivotToolEdModeToolkit::FPivotToolEdModeToolkit()
{
}

void FPivotToolEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	SAssignNew(ToolkitWidget, SPivotToolWidget);

	FModeToolkit::Init(InitToolkitHost);
}

FName FPivotToolEdModeToolkit::GetToolkitFName() const
{
	return FName("PivotToolEdMode");
}

FText FPivotToolEdModeToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("PivotToolEdModeToolkit", "DisplayName", "PivotToolEdMode Tool");
}

class FEdMode* FPivotToolEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FPivotToolEdMode::EM_PivotToolEdModeId);
}

#undef LOCTEXT_NAMESPACE
