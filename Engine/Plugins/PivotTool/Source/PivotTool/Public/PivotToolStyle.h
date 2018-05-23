// Copyright 2018 BruceLee, Inc. All Rights Reserved.
#pragma once

#include "Styling/SlateStyle.h"

class FPivotToolStyle
{
public:

	static void Initialize();

	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	/** @return The Slate style set */
	static const ISlateStyle& Get();

	static FName GetStyleSetName();

	static void SetupStyles(FSlateStyleSet* Style);
	static void SetupFlatButtonStyles(FSlateStyleSet* Style, const FName Name, const FLinearColor& Color);

private:

	static TSharedRef< class FSlateStyleSet > Create();

private:

	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};