// Copyright 2018 BruceLee, Inc. All Rights Reserved.

#pragma once

#include "PivotToolUtil.h"

#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateTypes.h"

//#include "SPivotPreset.generated.h"

UENUM()
enum EPivotToolPresetType
{
	PresetType_BoundCenter UMETA(DisplayName = "Boudnig Box Center"),
	PresetType_Center UMETA(DisplayName = "Center"),
	PresetType_Corner UMETA(DisplayName = "Corner"),
	PresetType_Edge UMETA(DisplayName = "Edge"),
};

UENUM()
enum EPivotToolPresetUIOrientation
{
	Horizontal,
	Vertical,
};

DECLARE_DELEGATE_RetVal_OneParam(FReply, FOnSelected, EPivotPreset)
DECLARE_DELEGATE_OneParam(FOnHovered, EPivotPreset)
DECLARE_DELEGATE_OneParam(FOnUnhovered, EPivotPreset)

class SPivotPreset : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SPivotPreset)
		:_OnSelected()
		, _OnHovered()
		, _OnUnhovered()
	{}
	SLATE_EVENT(FOnSelected, OnSelected)
		SLATE_EVENT(FOnHovered, OnHovered)
		SLATE_EVENT(FOnUnhovered, OnUnhovered)
		SLATE_END_ARGS()

		SPivotPreset();

public:
	void Construct(const FArguments& InArgs);

	TSharedRef<SWidget> CreatePresetButtons(EPivotToolPresetType InPresetType);

	ECheckBoxState IsAutoSave() const;
	ECheckBoxState IsGroupMode() const;
	ECheckBoxState IsVertexMode() const;
	ECheckBoxState IsChildrenMode() const;

protected:

	void OnOptionsDisplayedCheckStateChanged(ECheckBoxState NewCHeckedState);
	void OnAutoSaveCheckStateChanged(ECheckBoxState NewCHeckedState);
	void OnGroupModeCheckStateChanged(ECheckBoxState NewCHeckedState);
	void OnVertexModeCheckStateChanged(ECheckBoxState NewCHeckedState);
	void OnChildrenModeCheckStateChanged(ECheckBoxState NewCHeckedState);


	FOnSelected OnSelected;
	FOnHovered OnHovered;
	FOnUnhovered OnUnhovered;

private:
	FReply OnClickPresetButton(EPivotPreset InPivotPreset);
	void OnHoverPresetButton(EPivotPreset InPivotPreset);
	void OnUnhoverPresetButton(EPivotPreset InPivotPreset);

	TSharedRef<SWidget> CreatePresetUIByOrientation();
	TSharedRef<SWidget> CreatePresetOverlay(EPivotToolPresetType InPresetType);
	const FSlateBrush* GetPivotPresetBackgroundBrush(EPivotToolPresetType InPresetType);
	const FButtonStyle*  GetPivotPresetButtonStyle(EPivotToolPresetType InPresetType, EPivotPreset InPreset = EPivotPreset::MAX);
	const FVector2D GetPivotPresetButtonSize(EPivotToolPresetType InPresetType, EPivotPreset InPreset = EPivotPreset::MAX);


	void SaveUISetting();
	void LoadUISetting();

	static TArray<FVector2D> PivotPoints;


	bool bAutoSave;
	bool bGroupMode;
	bool bVertexMode;
	bool bChildrenMode;

	bool bVerticalUI;
};