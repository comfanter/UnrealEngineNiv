// Copyright 2018 BruceLee, Inc. All Rights Reserved.

#include "PivotToolEdMode.h"
#include "PivotToolEdModeToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"

const FEditorModeID FPivotToolEdMode::EM_PivotToolEdModeId = TEXT("EM_PivotToolEdMode");

FPivotToolEdMode::FPivotToolEdMode()
{

}

FPivotToolEdMode::~FPivotToolEdMode()
{

}

void FPivotToolEdMode::Enter()
{
	FEdMode::Enter();

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		Toolkit = MakeShareable(new FPivotToolEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}
}

void FPivotToolEdMode::Exit()
{
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

bool FPivotToolEdMode::UsesToolkits() const
{
	return true;
}




