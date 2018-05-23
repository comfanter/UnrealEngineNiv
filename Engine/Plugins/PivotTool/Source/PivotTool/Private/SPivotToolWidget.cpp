// Copyright 2018 BruceLee, Inc. All Rights Reserved.

#include "SPivotToolWidget.h"
#include "PivotToolUtil.h"
#include "PivotToolStyle.h"
#include "PivotTool.h"

#include "LevelEditorActions.h"
#include "EditorModeManager.h"
#include "StaticMeshResources.h"
#include "RawMesh.h"
#include "EditorSupportDelegates.h"
#include "Interfaces/IPluginManager.h"
#include "Engine/Selection.h"
#include "Engine/StaticMeshActor.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "SPivotToolWidget"

#define HORIZONTAL_SPACER + SHorizontalBox::Slot().FillWidth(1.f) [ SNew(SSpacer) ]
#define VERTICAL_SPACER + SVerticalBox::Slot().FillHeight(1.f) [ SNew(SSpacer) ]

void SPivotToolWidget::Construct(const FArguments& InArgs)
{
	LoadUISetting();

	PivotOffsetInput = FVector::ZeroVector;

	bIsCtrlDown = false;
	bIsShiftDown = false;
	bIsAltDown = false;

	bAlternativeButtons = false;
	bCanToggleAlternativeButton = true;

	PreviewPivots.Empty();

	const FText VersionName = FText::FromString(IPluginManager::Get().FindPlugin(TEXT("PivotTool"))->GetDescriptor().VersionName);
	const FText CreatedBy = FText::FromString(TEXT("by ") + IPluginManager::Get().FindPlugin(TEXT("PivotTool"))->GetDescriptor().CreatedBy);
	const FText FriendlyName = FText::FromString(IPluginManager::Get().FindPlugin(TEXT("PivotTool"))->GetDescriptor().FriendlyName);

	const FText PivotToolVersion = FText::Format(LOCTEXT("PivotToolVersion", "{0} {1}"), FriendlyName, VersionName);

	const float ButtonContentPadding = 4.f;
	const float ButtonTextMinDesiredWidth = 50.f;

	ChildSlot
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.4f, 0.4f, 0.4f))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(0.f)
			[
				SNew(SVerticalBox)

				#pragma region PivotPreset
				+ SVerticalBox::Slot()
				.Padding(0, 3.f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					[
						SNew(SBorder)
						//.BorderBackgroundColor(FLinearColor(.1f,.1f,.1f))
						.BorderBackgroundColor(FLinearColor(0.4f, 0.4f, 0.4f))
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(2.0f)
						[
							SAssignNew(PivotPreset, SPivotPreset)
							.OnSelected(this, &SPivotToolWidget::OnPivotPresetSelected)
							.OnHovered(this, &SPivotToolWidget::OnPivotPresetHovered)
							.OnUnhovered(this, &SPivotToolWidget::OnPivotPresetUnhovered)
						]
					]
				]
				#pragma endregion PivotPreset

				#pragma region PivotOffset
				+ SVerticalBox::Slot()
				.Padding(0, 3.f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.Padding(2, 0)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)

						#pragma region Offset Input
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 0)
						[
							SNew(SVectorInputBox)
							.bColorAxisLabels(true)
							.AllowResponsiveLayout(false)
							//.AllowSpin(true)
							.X(this, &SPivotToolWidget::GetPivotOffsetInputX)
							.Y(this, &SPivotToolWidget::GetPivotOffsetInputY)
							.Z(this, &SPivotToolWidget::GetPivotOffsetInputZ)
							.OnXCommitted(this, &SPivotToolWidget::OnSetPivotOffsetInput, 0)
							.OnYCommitted(this, &SPivotToolWidget::OnSetPivotOffsetInput, 1)
							.OnZCommitted(this, &SPivotToolWidget::OnSetPivotOffsetInput, 2)
						]
						#pragma endregion Offset Input

						#pragma region  Apply Offset Button
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 8)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.ContentPadding(ButtonContentPadding)
							.ButtonStyle(&FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.Round.DarkGray"))
							.OnClicked(this, &SPivotToolWidget::OnApplyOffsetClicked)
							.IsEnabled(this, &SPivotToolWidget::IsApplyOffsetEnabled)
							.ToolTipText(LOCTEXT("ApplyOffsetButtonTooltip", "Apply offset to pivot"))
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.TextStyle(FPivotToolStyle::Get(), "PivotTool.ButtonText")
								.Text(LOCTEXT("Offset", "Offset"))
								.MinDesiredWidth(ButtonTextMinDesiredWidth)
								.Justification(ETextJustify::Center)
							]
						]
						#pragma endregion  Apply Offset Button

						
						#pragma region Snap to Vertex
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 8)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.ContentPadding(ButtonContentPadding)
							.ButtonStyle(&FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.Round.DarkGray"))
							.OnClicked(this, &SPivotToolWidget::OnSnapToVertexClicked)
							.IsEnabled(this, &SPivotToolWidget::IsSnapToVertexEnabled)
							.ToolTipText(LOCTEXT("SnapToVertexButtonTooltip", "Snap pivot to closest vertex postion"))
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.TextStyle(FPivotToolStyle::Get(), "PivotTool.ButtonText")
								.Text(LOCTEXT("SnapToVertex", "Snap to Vert"))
								.MinDesiredWidth(ButtonTextMinDesiredWidth)
								.Justification(ETextJustify::Center)
							]
						]
						#pragma endregion Snap to Vertex
					]
				]
#pragma endregion PivotOffset

				#pragma region ToolBar-1
				+ SVerticalBox::Slot()
					.Padding(0, 3.f)
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.4f, 0.4f, 0.4f))
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.HAlign(HAlign_Center)
						.Padding(2.0f)
						[
							SNew(SHorizontalBox)

							#pragma region Set
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(2, 8)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.ContentPadding(ButtonContentPadding)
								.ButtonStyle(&FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.Round.Black"))
								.OnClicked(this, &SPivotToolWidget::OnSaveClicked)
								.ToolTipText(LOCTEXT("SaveButtonTooltip", "Save temporary actor pivot placement (Alt + Middle mouse)"))
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.TextStyle(FPivotToolStyle::Get(), "PivotTool.ButtonText")
									.Text(LOCTEXT("Set", "Set"))
									.MinDesiredWidth(ButtonTextMinDesiredWidth)
									.Justification(ETextJustify::Center)
								]
							]
							#pragma endregion Set

							#pragma region Reset
							+SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(2, 8)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.ContentPadding(ButtonContentPadding)
								.ButtonStyle(&FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.Round.Black"))
								.OnClicked(this, &SPivotToolWidget::OnResetClicked)
								.ToolTipText(LOCTEXT("ResetButtonTooltip", "Reset actor pivot"))
								[
									SNew(STextBlock)
									.TextStyle(FPivotToolStyle::Get(), "PivotTool.ButtonText")
									.Text(LOCTEXT("Reset", "Reset"))
									.MinDesiredWidth(ButtonTextMinDesiredWidth)
									.Justification(ETextJustify::Center)
								]
							]
							#pragma endregion reset
							
							#pragma region Copy
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(2, 8)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.ContentPadding(ButtonContentPadding)
								.ButtonStyle(&FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.Round.Black"))
								.OnClicked(this, &SPivotToolWidget::OnCopyClicked)
								.ToolTipText(LOCTEXT("CopyButtonTooltip", "Copy actor pivot"))
								[
									SNew(STextBlock)
									.TextStyle(FPivotToolStyle::Get(), "PivotTool.ButtonText")
								.Text(LOCTEXT("Copy", "Copy"))
								.MinDesiredWidth(ButtonTextMinDesiredWidth)
								.Justification(ETextJustify::Center)
								]
							]
							#pragma endregion Copy

							#pragma region Paste
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(2, 8)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.ContentPadding(ButtonContentPadding)
								.ButtonStyle(&FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.Round.Black"))
								.OnClicked(this, &SPivotToolWidget::OnPasteClicked)
								.IsEnabled(this, &SPivotToolWidget::IsPasteEnabled)
								.ToolTipText(LOCTEXT("PasteButtonTooltip", "Paste actor pivot"))
								[
									SNew(STextBlock)
									.TextStyle(FPivotToolStyle::Get(), "PivotTool.ButtonText")
								.Text(LOCTEXT("Paste", "Paste"))
								.MinDesiredWidth(ButtonTextMinDesiredWidth)
								.Justification(ETextJustify::Center)
								]
							]
							#pragma endregion Paste
						]
					]
					#pragma endregion ToolBar-1

				#pragma region ToolBar-2
				+ SVerticalBox::Slot()
				.Padding(0, 3.f)
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.4f, 0.4f, 0.4f))
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.HAlign(HAlign_Center)
					.Padding(2.0f)
					[
						SNew(SHorizontalBox)

					#pragma region Bake
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 8)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.ContentPadding(ButtonContentPadding)
							.ButtonStyle(&FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.Round.Black"))
							.OnClicked(this, &SPivotToolWidget::OnBakeClicked)
							.IsEnabled(this, &SPivotToolWidget::IsBakeEnabled)
							.ToolTipText(LOCTEXT("BakeButtonTooltip", "Bake last selected static mesh actor's pivot to static mesh.\nYou can not undo this operation!"))
							[
								SNew(STextBlock)
								.TextStyle(FPivotToolStyle::Get(), "PivotTool.ButtonText")
								.Text(LOCTEXT("Bake", "Bake"))
								.MinDesiredWidth(ButtonTextMinDesiredWidth)
								.Justification(ETextJustify::Center)
							]
						]
					#pragma endregion Bake

					#pragma region Duplicate And Bake
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 8)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.ContentPadding(ButtonContentPadding)
							.ButtonStyle(&FPivotToolStyle::Get().GetWidgetStyle<FButtonStyle>("PivotTool.ButtonStyle.Round.Black"))
							.OnClicked(this, &SPivotToolWidget::OnDuplicateAndBakeClicked)
							.IsEnabled(this, &SPivotToolWidget::IsBakeEnabled)
							.ToolTipText(LOCTEXT("DuplicateAndBakeButtonTooltip", "Duplicate static mesh and bake last selected static mesh actor's pivot to duplicated static mesh.\nYou can not undo this operation!"))
							[
								SNew(STextBlock)
								.TextStyle(FPivotToolStyle::Get(), "PivotTool.ButtonText")
								.Text(LOCTEXT("DuplicateAndBake", "Dup & Bake"))
								.MinDesiredWidth(90.0f)
								.Justification(ETextJustify::Center)
							]
						]
					#pragma endregion Duplicate And Bake

					#pragma region FreezeRotation
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 8)
						[
							SNew(SCheckBox)
							//.Style(&FPivotToolStyle::Get().GetWidgetStyle<FCheckBoxStyle>("PivotTool.ToggleButton"))
							.OnCheckStateChanged(this, &SPivotToolWidget::OnFreezeRotationCheckStateChanged)
							.IsChecked(this, &SPivotToolWidget::IsFreezeRotation)
							.ToolTipText(LOCTEXT("FreezeRotationTip", "Also freeze rotation when baking pivot"))
							.Content()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("FreezeRotaion", "Freeze Rotation"))
								.TextStyle(FPivotToolStyle::Get(), "PivotTool.CheckboxText")
								.Justification(ETextJustify::Center)
							]
						]
					#pragma endregion FreezeRotation

					]

				]
				#pragma endregion ToolBar-2

				VERTICAL_SPACER

				+ SVerticalBox::Slot()
				.Padding(0, 3.f)
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.Padding(2, 0)
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.Padding(0)
						.AutoHeight()
						[
							SNew(SImage)
							.Image(FPivotToolStyle::Get().GetBrush("PivotTool.PivotToolText"))
						]
					]

					+ SHorizontalBox::Slot()
					.Padding(2, 0)
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						//.TextStyle(FPivotToolStyle::Get(), "PivotTool.ButtonText")
						.Text(VersionName)
					]
				]
			]

			
		];
}

bool SPivotToolWidget::SupportsKeyboardFocus() const
{
	return true;
}

FReply SPivotToolWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = FReply::Unhandled();
	//if (IsEnabled())
	{
		if (InKeyEvent.GetKey() == EKeys::LeftControl || InKeyEvent.GetKey() == EKeys::RightControl)
		{
			bIsCtrlDown = true;
			Reply = FReply::Handled();
			if (bCanToggleAlternativeButton)
			{
				bAlternativeButtons = !bAlternativeButtons;
				bCanToggleAlternativeButton = false;
			}
			//Invalidate(EInvalidateWidget::Layout);
		}
		if (InKeyEvent.GetKey() == EKeys::LeftShift || InKeyEvent.GetKey() == EKeys::RightShift)
		{
			bIsShiftDown = true;
			Reply = FReply::Handled();
			//Invalidate(EInvalidateWidget::Layout);
		}
		if (InKeyEvent.GetKey() == EKeys::LeftAlt || InKeyEvent.GetKey() == EKeys::RightAlt)
		{
			bIsAltDown = true;
			Reply = FReply::Handled();
			//Invalidate(EInvalidateWidget::Layout);
		}
	}
	return Reply;
}

FReply SPivotToolWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = FReply::Unhandled();

	//if (IsEnabled())
	{
		if (InKeyEvent.GetKey() == EKeys::LeftControl || InKeyEvent.GetKey() == EKeys::RightControl)
		{
			bIsCtrlDown = false;
			Reply = FReply::Handled();
			//			bCanToggleAlternativeButton = true;
		}
		if (InKeyEvent.GetKey() == EKeys::LeftShift || InKeyEvent.GetKey() == EKeys::RightShift)
		{
			bIsShiftDown = false;
			Reply = FReply::Handled();
			//Invalidate(EInvalidateWidget::Layout);
		}
		if (InKeyEvent.GetKey() == EKeys::LeftAlt || InKeyEvent.GetKey() == EKeys::RightAlt)
		{
			bIsAltDown = false;
			Reply = FReply::Handled();
			//Invalidate(EInvalidateWidget::Layout);
		}
	}

	return Reply;
}

void SPivotToolWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	TickPreviewPivots();
}

void SPivotToolWidget::SaveUISetting()
{
	//GConfig->SetBool(TEXT("PivotTool"), TEXT("bFreezeRotation"), bFreezeRotation, GEditorPerProjectIni);
}

void SPivotToolWidget::LoadUISetting()
{
	//GConfig->GetBool(TEXT("PivotTool"), TEXT("bFreezeRotation"), bFreezeRotation, GEditorPerProjectIni);
}

FReply SPivotToolWidget::OnApplyOffsetClicked()
{
	ECoordSystem CoordSystem = GLevelEditorModeTools().GetCoordSystem();
	bool bWorldSpace = CoordSystem == COORD_World;
	FPivotToolUtil::OffsetSelectedActorsPivotOffset(PivotOffsetInput, bWorldSpace);
	return FReply::Handled();
}

bool SPivotToolWidget::IsApplyOffsetEnabled() const
{
	return !PivotOffsetInput.IsNearlyZero() && GEditor->GetSelectedActorCount() > 0;
}

FReply SPivotToolWidget::OnSnapToVertexClicked()
{
	SPivotToolWidget::OnSaveClicked();
	FPivotToolUtil::SnapSelectedActorsPivotToVertex();
	return FReply::Handled();
}

bool SPivotToolWidget::IsSnapToVertexEnabled() const
{
	return GEditor->GetSelectedActorCount() > 0;
}

FReply SPivotToolWidget::OnSaveClicked()
{
	FEditorModeTools& EditorModeTools = GLevelEditorModeTools();
	FPivotToolUtil::UpdateSelectedActorsPivotOffset(EditorModeTools.PivotLocation, /*bWorldSpace=*/ true);
	return FReply::Handled();
}

FReply SPivotToolWidget::OnCopyClicked()
{
	FPivotToolUtil::CopyActorPivotOffset();
	return FReply::Handled();
}

FReply SPivotToolWidget::OnPasteClicked()
{
	FPivotToolUtil::PasteActorPivotOffset();
	return FReply::Handled();
}

bool SPivotToolWidget::IsPasteEnabled() const
{
	return FPivotToolUtil::HasPivotOffsetCopied();
}

// FText SPivotToolWidget::GetResetButtonText() const
// {
// 	return bAlternativeButtons
// 		? LOCTEXT("ResetToWorldZero", "Reset to World Zero")
// 		: LOCTEXT("Reset", "Reset");
// }
// 
// FText SPivotToolWidget::GetResetButtonHintText() const
// {
// 	return bAlternativeButtons
// 		? LOCTEXT("ResetToWorldZeroButtonTooltip", "Reset actor pivot offset to zero vector in world space\nPress Ctrl key to switch to \"Reset\"")
// 		: LOCTEXT("ResetButtonTooltip", "Reset actor pivot offset\nPress Ctrl key to switch to \"Reset to World Space\"");
// }

FReply SPivotToolWidget::OnResetClicked()
{
	//bool bWorldSpace = bAlternativeButtons;
	const bool bWorldSpace = false;
	FPivotToolUtil::UpdateSelectedActorsPivotOffset(FVector::ZeroVector, bWorldSpace);
	return FReply::Handled();
}

// FText SPivotToolWidget::GetBakeButtonText() const
// {
// 	return bAlternativeButtons
// 		? LOCTEXT("Bake", "Duplicate and Bake")
// 		: LOCTEXT("Bake", "Bake");
// }
// 
// FText SPivotToolWidget::GetBakeButtonHintText() const
// {
// 	return bAlternativeButtons
// 		? LOCTEXT("DuplicateAndBakeButtonTooltip", "Duplicate static mesh and bake last selected static mesh actor's pivot offset to duplicated static mesh\nPress Ctrl key to switch to \"Bake\"")
// 		: LOCTEXT("BakeButtonTooltip", "Bake last selected static mesh actor's pivot offset to static mesh\nPress Ctrl key to switch to \"Duplicate and Bake\"");
// }

FReply SPivotToolWidget::OnBakeClicked()
{
	const bool bDuplicate = false;

	USelection* SelectedActors = GEditor->GetSelectedActors();
	int32 SucceedCount = 0;
	TArray<UStaticMesh*> BakedMeshes;

	bool bSilentMode = SelectedActors->Num() > 1;
	for (FSelectionIterator SelectionIt(*SelectedActors); SelectionIt; ++SelectionIt)
	{
		if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(*SelectionIt))
		{
			if (UStaticMesh* StaticMesh = StaticMeshActor->GetStaticMeshComponent()->GetStaticMesh())
			{
				if (BakedMeshes.Find(StaticMesh) == INDEX_NONE)
				{
					if (FPivotToolUtil::BakeStaticMeshActorsPivotOffsetToRawMesh(StaticMeshActor, /*bBakeRotation=*/ bFreezeRotation, /*bDuplicate=*/ bDuplicate, /*bSilentMode=*/ bSilentMode))
					{
						++SucceedCount;
					}
					BakedMeshes.AddUnique(StaticMesh);
				}
				else
				{
					// Reset Pivot Offset if Mesh Pivot already baked
					FPivotToolUtil::UpdateActorPivotOffset(StaticMeshActor, FVector::ZeroVector, /*bWorldSpace=*/ false);
				}
			}
		}
	}

	FPivotToolUtil::NotifyMessage(FText::FromString(FString::Printf(TEXT("%d Static Mesh Baked!"), SucceedCount)), 0.5f);

	// 	AStaticMeshActor* StaticMeshActor = GEditor->GetSelectedActors()->GetBottom<AStaticMeshActor>();
	// 	if (StaticMeshActor)
	// 	{
	// 		//bool bDuplicate = bAlternativeButtons;
	// 		const bool bDuplicate = false;
	// 		FPivotToolUtil::BakeStaticMeshActorsPivotOffsetToRawMesh(StaticMeshActor, /*bBakeRotation=*/ bFreezeRotation, /*bDuplicate=*/ bDuplicate);
	// 	}
	// 	else
	// 	{
	// 		FPivotToolUtil::NotifyMessage(LOCTEXT("NoStaticMeshActor", "Can Only Bake to Static Mesh Actor! "), 0.5f);
	// 	}

	return FReply::Handled();
}

FReply SPivotToolWidget::OnDuplicateAndBakeClicked()
{
	const bool bDuplicate = true;

	USelection* SelectedActors = GEditor->GetSelectedActors();
	int32 SucceedCount = 0;
	for (FSelectionIterator SelectionIt(*SelectedActors); SelectionIt; ++SelectionIt)
	{
		AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(*SelectionIt);
		if (StaticMeshActor)
		{
			if (FPivotToolUtil::BakeStaticMeshActorsPivotOffsetToRawMesh(StaticMeshActor, /*bBakeRotation=*/ bFreezeRotation, /*bDuplicate=*/ bDuplicate))
			{
				++SucceedCount;
			}
		}
	}

	FPivotToolUtil::NotifyMessage(FText::FromString(FString::Printf(TEXT("%d Static Mesh Actor Baked!"), SucceedCount)), 0.5f);

	// 	AStaticMeshActor* StaticMeshActor = GEditor->GetSelectedActors()->GetBottom<AStaticMeshActor>();
	// 	if (StaticMeshActor)
	// 	{
	// 		const bool bDuplicate = true;
	// 		FPivotToolUtil::BakeStaticMeshActorsPivotOffsetToRawMesh(StaticMeshActor, /*bBakeRotation=*/ bFreezeRotation, /*bDuplicate=*/ bDuplicate);
	// 	}
	// 	else
	// 	{
	// 		FPivotToolUtil::NotifyMessage(LOCTEXT("NoStaticMeshActor", "Can Only Bake to Static Mesh Actor! "), 0.5f);
	// 	}

	return FReply::Handled();
}

bool SPivotToolWidget::IsBakeEnabled() const
{
	AStaticMeshActor* StaticMeshActor = GEditor->GetSelectedActors()->GetBottom<AStaticMeshActor>();
	return StaticMeshActor != NULL;
}

ECheckBoxState SPivotToolWidget::IsFreezeRotation() const
{
	return bFreezeRotation ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SPivotToolWidget::OnFreezeRotationCheckStateChanged(ECheckBoxState NewCHeckedState)
{
	bFreezeRotation = NewCHeckedState == ECheckBoxState::Checked;
	SaveUISetting();
}

FReply SPivotToolWidget::OnPivotPresetSelected(EPivotPreset InPreset)
{
#if PIVOT_TOOL_DEBUG
	UE_LOG(LogPivotTool, Display, TEXT("SPivotToolWidget::OnPivotPresetSelected: %d"), (int32)InPreset);
#endif
	const bool bAutoSave = PivotPreset->IsAutoSave() == ECheckBoxState::Checked;
	const bool bGroupMode = PivotPreset->IsGroupMode() == ECheckBoxState::Checked;
	const bool bVertexMode = PivotPreset->IsVertexMode() == ECheckBoxState::Checked;
	const bool bChildrenMode = PivotPreset->IsChildrenMode() == ECheckBoxState::Checked;

	FPivotToolUtil::UpdateSelectedActorsPivotOffset(InPreset, /*bAutoSave=*/ bAutoSave, /*bAsGroup=*/ bGroupMode, /*bVertexMode=*/ bVertexMode, /*bChildrenMode=*/ bChildrenMode);
	return FReply::Handled();
}

void SPivotToolWidget::OnPivotPresetHovered(EPivotPreset InPreset)
{
#if PIVOT_TOOL_DEBUG
	UE_LOG(LogPivotTool, Display, TEXT("SPivotToolWidget::OnPivotPresetHovered: %d"), (int32)InPreset);
#endif
	const bool bAutoSave = PivotPreset->IsAutoSave() == ECheckBoxState::Checked;
	const bool bGroupMode = PivotPreset->IsGroupMode() == ECheckBoxState::Checked;
	const bool bVertexMode = PivotPreset->IsVertexMode() == ECheckBoxState::Checked;
	const bool bChildrenMode = PivotPreset->IsChildrenMode() == ECheckBoxState::Checked;

	FPivotToolUtil::GetPreviewPivotsOfSelectedActors(InPreset, /*bAutoSave=*/ bAutoSave, /*bAsGroup=*/ bGroupMode, /*bVertexMode=*/ bVertexMode, /*bChildrenMode=*/ bChildrenMode, PreviewPivots);
}

void SPivotToolWidget::OnPivotPresetUnhovered(EPivotPreset InPreset)
{
#if PIVOT_TOOL_DEBUG
	UE_LOG(LogPivotTool, Display, TEXT("SPivotToolWidget::OnPivotPresetUnhovered: %d"), (int32)InPreset);
#endif
	PreviewPivots.Empty();
}

void SPivotToolWidget::OnSetPivotOffsetInput(float InValue, ETextCommit::Type CommitType, int32 InAxis)
{
	if (InAxis == 0)
	{
		PivotOffsetInput.X = InValue;
	}
	else if (InAxis == 1)
	{
		PivotOffsetInput.Y = InValue;
	}
	else if (InAxis == 2)
	{
		PivotOffsetInput.Z = InValue;
	}
}

void SPivotToolWidget::TickPreviewPivots()
{
	const UPivotToolSettings* PivotToolSettings = GetDefault<UPivotToolSettings>();

	if (!PivotToolSettings->bDisplayPivotPresetPreview)
	{
		return;
	}

	if (GEditor->GetSelectedActorCount() <= 0)
	{
		return;
	}

	if (PreviewPivots.Num() <= 0)
	{
		return;
	}

	if (AActor* LastSelectedActor = GEditor->GetSelectedActors()->GetBottom<AActor>())
	{
		if (UWorld* World = LastSelectedActor->GetWorld())
		{
			const FColor SphereColor = PivotToolSettings->PivotPresetPreviewColor;
			const float SphereRadius = PivotToolSettings->PivotPresetPreviewSize;
			const int32 SphereSegments = 12;

			const float LineThickness = PivotToolSettings->PivotPresetPreviewLineThickness;

			// 			const float PointSize = 10.f;
			// 			const FColor PointColor = FColor::Green;

			const float CoordScale = SphereRadius * 2;

			for (auto It = PreviewPivots.CreateConstIterator(); It; ++It)
			{
				const FTransform& Pivot = *It;
				//DrawDebugPoint(World, Pivot.GetLocation(), PointSize, PointColor, false, -1.f, SDPG_Foreground);
				DrawDebugSphere(World, Pivot.GetLocation(), SphereRadius, SphereSegments, SphereColor, false, -1, SDPG_Foreground, LineThickness);
				DrawDebugCoordinateSystem(World, Pivot.GetLocation(), Pivot.Rotator(), CoordScale, true, -1.f, SDPG_Foreground, LineThickness);
			}
		}
	}
}

#undef HORIZONTAL_SPACER
#undef VERTICAL_SPACER

#undef LOCTEXT_NAMESPACE
