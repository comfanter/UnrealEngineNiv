// Copyright 2007-2017 Simul Software Ltd.. All Rights Reserved.

#if UE_EDITOR
#include "AssetTypeActions_TrueSkySequence.h"
#include "TrueSkyEditorPluginPrivatePCH.h"
#include "TrueSkySequenceAsset.h"

void FAssetTypeActions_TrueSkySequence::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor )
{
	if ( InObjects.Num() > 0 )
	{
		if ( UTrueSkySequenceAsset* const TrueSkyAsset = Cast<UTrueSkySequenceAsset>(InObjects[0]) )
		{
			ITrueSkyEditorPlugin::Get().OpenEditor( TrueSkyAsset );
		}
	}
}
#endif
