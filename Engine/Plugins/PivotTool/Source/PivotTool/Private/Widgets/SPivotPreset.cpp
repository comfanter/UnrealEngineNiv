// Copyright 2018 BruceLee, Inc. All Rights Reserved.

#include "SPivotPreset.h"
#include "PivotToolUtil.h"
#include "PivotToolStyle.h"
#include "SAlphaButton.h"
#include "PivotTool.h"

#include "LevelEditorActions.h"
#include "EditorModeManager.h"
#include "StaticMeshResources.h"
#include "RawMesh.h"
#include "EditorSupportDelegates.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"


#define LOCTEXT_NAMESPACE "SPivotToolWidget"

// 160x160
TArray<FVector2D> SPivotPreset::PivotPoints = { { 82, 74 },
// Center
{ 82, 36 },{ 83, 113 },{ 45, 72 },{ 121, 82 },{ 98, 63 },{ 65, 88 },
//Corner
{ 27, 46 },{ 63, 21 },{ 135, 28 },{ 105, 55 },{ 27, 123 },{ 62, 92 },{ 136, 103 },{ 104, 137 },
//Edge
{ 44, 37 },{ 96, 29 },{ 117, 43 },{ 64, 53 },{ 27, 84 },{ 62, 59 },{ 129, 67 },{ 100, 95 },{ 44, 106 },{ 96, 97 },{ 117, 118 },{ 64, 127 },
};

SPivotPreset::SPivotPreset()
	: bAutoSave(true)
	, bGroupMode(false)
	, bVertexMode(false)
	, bChildrenMode(false)

	, bVerticalUI(false)
{
}


void SPivotPreset::Construct(const FArguments& InArgs)
{
	LoadUISetting();

	OnSelected = InArgs._OnSelected;
	OnHovered = InArgs._OnHovered;
	OnUnhovered = InArgs._OnUnhovered;

	ChildSlot
		[
			SNew(SVerticalBox)

#pragma region Options
			+ SVerticalBox::Slot()
			.Padding(0, 3)
			.AutoHeight()
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.Padding(2, 0)
				[

					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.Padding(2, 0)
					[
						SNew(SCheckBox)
						.Style(&FPivotToolStyle::Get().GetWidgetStyle<FCheckBoxStyle>("PivotTool.ToggleButton"))
						.OnCheckStateChanged(this, &SPivotPreset::OnAutoSaveCheckStateChanged)
						.IsChecked(this, &SPivotPreset::IsAutoSave)
						.ToolTipText(LOCTEXT("AutoSaveToolTip", "Toggles whether auto save pivot offset"))
						.Content()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("AutoSave", "Auto Save"))
							.TextStyle(FPivotToolStyle::Get(), "PivotTool.CheckboxText")
							.Justification(ETextJustify::Center)
						]
					]

					+ SHorizontalBox::Slot()
					.Padding(2, 0)
					[
						SNew(SCheckBox)
						.Style(&FPivotToolStyle::Get().GetWidgetStyle<FCheckBoxStyle>("PivotTool.ToggleButton"))
						.OnCheckStateChanged(this, &SPivotPreset::OnGroupModeCheckStateChanged)
						.IsChecked(this, &SPivotPreset::IsGroupMode)
						.ToolTipText(LOCTEXT("GroupModeToolTip", "Toggles whether treat selected actors as one group"))
						.Content()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("GroupMode", "Group Mode"))
							.TextStyle(FPivotToolStyle::Get(), "PivotTool.CheckboxText")
							.Justification(ETextJustify::Center)
						]
					]

					+ SHorizontalBox::Slot()
					.Padding(2, 0)
					[
						SNew(SCheckBox)
						.Style(&FPivotToolStyle::Get().GetWidgetStyle<FCheckBoxStyle>("PivotTool.ToggleButton"))
						.OnCheckStateChanged(this, &SPivotPreset::OnVertexModeCheckStateChanged)
						.IsChecked(this, &SPivotPreset::IsVertexMode)
						.ToolTipText(LOCTEXT("VertexModeToolTip", "Toggles wheter set pivot to vertex instead of bounding box"))
						.Content()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("VertexMode", "Vertex Mode"))
							.TextStyle(FPivotToolStyle::Get(), "PivotTool.CheckboxText")
							.Justification(ETextJustify::Center)
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(2, 0)
					[
						SNew(SCheckBox)
						.Style(&FPivotToolStyle::Get().GetWidgetStyle<FCheckBoxStyle>("PivotTool.ToggleButton"))
						.OnCheckStateChanged(this, &SPivotPreset::OnChildrenModeCheckStateChanged)
						.IsChecked(this, &SPivotPreset::IsChildrenMode)
						.ToolTipText(LOCTEXT("ChildrenModeToolTip", "Toggles whether use children actors to calculate bounding box"))
						.Content()
						[
						SNew(STextBlock)
						.Text(LOCTEXT("ChildrenMode", "Children Mode"))
						.TextStyle(FPivotToolStyle::Get(), "PivotTool.CheckboxText")
						.Justification(ETextJustify::Center)
						]
					]
				]	
			]
#pragma endregion
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.HAlign(HAlign_Center)
		[
			CreatePresetUIByOrientation()
		]

		];
}

ECheckBoxState SPivotPreset::IsAutoSave() const
{
	return bAutoSave ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SPivotPreset::IsGroupMode() const
{
	return bGroupMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SPivotPreset::IsVertexMode() const
{
	return bVertexMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SPivotPreset::IsChildrenMode() const
{
	return bChildrenMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SPivotPreset::OnAutoSaveCheckStateChanged(ECheckBoxState NewCHeckedState)
{
	bAutoSave = NewCHeckedState == ECheckBoxState::Checked;
	SaveUISetting();
}

void SPivotPreset::OnGroupModeCheckStateChanged(ECheckBoxState NewCHeckedState)
{
	bGroupMode = NewCHeckedState == ECheckBoxState::Checked;
	if (bGroupMode)
	{
		bVertexMode = false;
		bChildrenMode = false;
	}
	SaveUISetting();
}

void SPivotPreset::OnVertexModeCheckStateChanged(ECheckBoxState NewCHeckedState)
{
	bVertexMode = NewCHeckedState == ECheckBoxState::Checked;
	if (bVertexMode)
	{
		bGroupMode = false;
		bChildrenMode = false;
	}
	SaveUISetting();
}

void SPivotPreset::OnChildrenModeCheckStateChanged(ECheckBoxState NewCHeckedState)
{
	bChildrenMode = NewCHeckedState == ECheckBoxState::Checked;
	if (bChildrenMode)
	{
		bVertexMode = false;
		bGroupMode = false;
	}
	SaveUISetting();
}

FReply SPivotPreset::OnClickPresetButton(EPivotPreset InPivotPreset)
{
#if PIVOT_TOOL_DEBUG
	UE_LOG(LogPivotTool, Display, TEXT("SPivotPreset::OnClickPresetButton: %d"), (int32)InPivotPreset);
#endif
	if (OnSelected.IsBound())
	{
		OnSelected.Execute(InPivotPreset);
	}

	return FReply::Handled();
}

void SPivotPreset::OnHoverPresetButton(EPivotPreset InPivotPreset)
{
#if PIVOT_TOOL_DEBUG
	UE_LOG(LogPivotTool, Display, TEXT("SPivotPreset::OnHoverPresetButton: %d"), (int32)InPivotPreset);
#endif
	if (OnHovered.IsBound())
	{
		OnHovered.Execute(InPivotPreset);
	}
}

void SPivotPreset::OnUnhoverPresetButton(EPivotPreset InPivotPreset)
{
#if PIVOT_TOOL_DEBUG
	UE_LOG(LogPivotTool, Display, TEXT("SPivotPreset::OnUnhoverPresetButton: %d"), (int32)InPivotPreset);
#endif
	if (OnUnhovered.IsBound())
	{
		OnUnhovered.Execute(InPivotPreset);
	}
}

TSharedRef<SWidget> SPivotPreset::CreatePresetUIByOrientation()
{

	return
		SNew(SScrollBox)
		.Orientation(Orient_Horizontal)
		.ScrollBarAlwaysVisible(false)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]

				// Center
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreatePresetOverlay(EPivotToolPresetType::PresetType_BoundCenter)
				]

				+ SHorizontalBox::Slot().FillWidth(0.5)[SNew(SSpacer)]

				//Surface Center
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreatePresetOverlay(EPivotToolPresetType::PresetType_Center)
				]

				+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			]

			+ SVerticalBox::Slot()
			[

				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.5)[SNew(SSpacer)]

				// Corner
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreatePresetOverlay(EPivotToolPresetType::PresetType_Corner)
				]

				+ SHorizontalBox::Slot().FillWidth(0.5)[SNew(SSpacer)]

				// Edge
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreatePresetOverlay(EPivotToolPresetType::PresetType_Edge)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			]
		];
}

TSharedRef<SWidget> SPivotPreset::CreatePresetOverlay(EPivotToolPresetType InPresetType)
{
	return SNew(SOverlay)

		// Background
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SImage)
			.Image(GetPivotPresetBackgroundBrush(InPresetType))
		]

	// Widgets
	+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			CreatePresetButtons(InPresetType)
		];
}

const FSlateBrush* SPivotPreset::GetPivotPresetBackgroundBrush(EPivotToolPresetType InPresetType)
{
	switch (InPresetType)
	{
	case EPivotToolPresetType::PresetType_Corner:
		return FPivotToolStyle::Get().GetBrush("PivotTool.PivotPresetBackground.Corner");
	case EPivotToolPresetType::PresetType_Edge:
		return FPivotToolStyle::Get().GetBrush("PivotTool.PivotPresetBackground.Edge");
	case EPivotToolPresetType::PresetType_Center:
		return FPivotToolStyle::Get().GetBrush("PivotTool.PivotPresetBackground.Center");
	case EPivotToolPresetType::PresetType_BoundCenter:
	default:
		return FPivotToolStyle::Get().GetBrush("PivotTool.PivotPresetBackground.BoundCenter");
	}
}

const FButtonStyle*  SPivotPreset::GetPivotPresetButtonStyle(EPivotToolPresetType InPresetType, EPivotPreset InPreset)
{
	switch (InPresetType)
	{
	case EPivotToolPresetType::PresetType_Corner:
		return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotCorner");
	case EPivotToolPresetType::PresetType_Edge:
		if (InPreset == EPivotPreset::BoundingBoxEdgeTop1)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Top1");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeTop2)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Top2");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeTop3)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Top3");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeTop4)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Top4");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeMid1)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Mid1");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeMid2)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Mid2");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeMid3)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Mid3");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeMid4)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Mid4");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeBottom1)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Bottom1");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeBottom2)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Bottom2");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeBottom3)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Bottom3");
		}
		if (InPreset == EPivotPreset::BoundingBoxEdgeBottom4)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotEdge.Bottom4");
		}
		break;
	case EPivotToolPresetType::PresetType_Center:
		if (InPreset == EPivotPreset::BoundingBoxCenterTop)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotCenterTop");
		}
		if (InPreset == EPivotPreset::BoundingBoxCenterBottom)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotCenterBottom");
		}
		if (InPreset == EPivotPreset::BoundingBoxCenterLeft)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotCenterLeft");
		}
		if (InPreset == EPivotPreset::BoundingBoxCenterRight)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotCenterRight");
		}
		if (InPreset == EPivotPreset::BoundingBoxCenterFront)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotCenterFront");
		}
		if (InPreset == EPivotPreset::BoundingBoxCenterBack)
		{
			return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotCenterBack");
		}
		break;
	case EPivotToolPresetType::PresetType_BoundCenter:
	default:
		return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotBoundCenter");
	}
	return &FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.PivotBoundCenter");
}

const FVector2D SPivotPreset::GetPivotPresetButtonSize(EPivotToolPresetType InPresetType, EPivotPreset InPreset /*= EPivotPreset::MAX*/)
{
	switch (InPresetType)
	{
	case EPivotToolPresetType::PresetType_Edge:
		return GetPivotPresetButtonStyle(InPresetType, InPreset)->Normal.ImageSize;
	case EPivotToolPresetType::PresetType_Corner:
	case EPivotToolPresetType::PresetType_Center:
	case EPivotToolPresetType::PresetType_BoundCenter:
	default:
		return FVector2D(30.f, 30.f);
	}
}

void SPivotPreset::SaveUISetting()
{
	GConfig->SetBool(TEXT("PivotTool"), TEXT("bAutoSave"), bAutoSave, GEditorPerProjectIni);
	GConfig->SetBool(TEXT("PivotTool"), TEXT("bGroupMode"), bGroupMode, GEditorPerProjectIni);
	GConfig->SetBool(TEXT("PivotTool"), TEXT("bVertexMode"), bVertexMode, GEditorPerProjectIni);
	GConfig->SetBool(TEXT("PivotTool"), TEXT("bChildrenMode"), bChildrenMode, GEditorPerProjectIni);
}
void SPivotPreset::LoadUISetting()
{
	GConfig->GetBool(TEXT("PivotTool"), TEXT("bAutoSave"), bAutoSave, GEditorPerProjectIni);
	GConfig->GetBool(TEXT("PivotTool"), TEXT("bGroupMode"), bGroupMode, GEditorPerProjectIni);
	GConfig->GetBool(TEXT("PivotTool"), TEXT("bVertexMode"), bVertexMode, GEditorPerProjectIni);
	GConfig->GetBool(TEXT("PivotTool"), TEXT("bChildrenMode"), bChildrenMode, GEditorPerProjectIni);
}

TSharedRef<SWidget> SPivotPreset::CreatePresetButtons(EPivotToolPresetType InPresetType)
{
	int32 Start = 0;
	int32 End = 0;

	TArray<EPivotPreset> EdgeReOrder({
		EPivotPreset::BoundingBoxEdgeBottom2,
		EPivotPreset::BoundingBoxEdgeBottom1,
		EPivotPreset::BoundingBoxEdgeMid2,
		EPivotPreset::BoundingBoxEdgeTop2,
		EPivotPreset::BoundingBoxEdgeMid3,
		EPivotPreset::BoundingBoxEdgeTop1,
		EPivotPreset::BoundingBoxEdgeTop3,
		EPivotPreset::BoundingBoxEdgeBottom3,
		EPivotPreset::BoundingBoxEdgeBottom4,
		EPivotPreset::BoundingBoxEdgeMid1,
		EPivotPreset::BoundingBoxEdgeMid4,
		EPivotPreset::BoundingBoxEdgeTop4,
		});

	switch (InPresetType)
	{
	case EPivotToolPresetType::PresetType_Center:
		Start = (int32)EPivotPreset::BoundingBoxCenterTop;
		End = (int32)EPivotPreset::BoundingBoxCenterBack;
		break;
	case EPivotToolPresetType::PresetType_Corner:
		Start = (int32)EPivotPreset::BoundingBoxCornerTop1;
		End = (int32)EPivotPreset::BoundingBoxCornerBottom4;
		break;
	case EPivotToolPresetType::PresetType_Edge:
		Start = (int32)EPivotPreset::BoundingBoxEdgeTop1;
		End = (int32)EPivotPreset::BoundingBoxEdgeBottom4;
	case EPivotToolPresetType::PresetType_BoundCenter:
	default:
		break;
	}

	TSharedRef<SCanvas> Canvas = SNew(SCanvas);

	const float BGImgSize = 160.f;
	const float BGExpectedSize = 160.f;
	//	const float ButtonSize = 30.f;
	for (int32 EnumIndex = Start; EnumIndex <= End; ++EnumIndex)
	{
		if (InPresetType == EPivotToolPresetType::PresetType_Edge)
		{
			EPivotPreset ReOrderedPreset = EdgeReOrder[EnumIndex - Start];
			const FVector2D ButtonSize = GetPivotPresetButtonSize(InPresetType, ReOrderedPreset);

			Canvas->AddSlot()
				//.Position((PivotPoints[EnumIndex] - FVector2D(15, 15)) * (BGImgSize / BGExpectedSize))
				.Position((PivotPoints[(int32)ReOrderedPreset] - ButtonSize / 2) * (BGImgSize / BGExpectedSize))
				.Size(ButtonSize)
				[
					SNew(SAlphaButton)
					.ClickMethod(EButtonClickMethod::MouseDown)
				.ButtonStyle(GetPivotPresetButtonStyle(InPresetType, ReOrderedPreset))
				.OnClicked(this, &SPivotPreset::OnClickPresetButton, ReOrderedPreset)
				.OnHovered(this, &SPivotPreset::OnHoverPresetButton, ReOrderedPreset)
				.OnUnhovered(this, &SPivotPreset::OnUnhoverPresetButton, ReOrderedPreset)
				];
		}
		else
		{
			const FVector2D ButtonSize = GetPivotPresetButtonSize(InPresetType, EPivotPreset(EnumIndex));
			Canvas->AddSlot()
				.Position((PivotPoints[EnumIndex] - FVector2D(15, 15)) * (BGImgSize / BGExpectedSize))
				.Size(ButtonSize)
				[
					SNew(SButton)
					.ClickMethod(EButtonClickMethod::MouseDown)
				.ButtonStyle(GetPivotPresetButtonStyle(InPresetType, EPivotPreset(EnumIndex)))
				.OnClicked(this, &SPivotPreset::OnClickPresetButton, EPivotPreset(EnumIndex))
				.OnHovered(this, &SPivotPreset::OnHoverPresetButton, EPivotPreset(EnumIndex))
				.OnUnhovered(this, &SPivotPreset::OnUnhoverPresetButton, EPivotPreset(EnumIndex))
				];
		}
	}

	return Canvas;
}
