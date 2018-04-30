
#include "TrueSkyWaterComponent.h"

#include "TrueSkyPluginPrivatePCH.h"

#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#endif
#include "MessageLog.h"
#include "UObjectToken.h"
#include "Misc/ScopeLock.h"
#include "MapErrors.h"
#include "ComponentInstanceDataCache.h"
#include "ShaderCompiler.h"
#include "SceneManagement.h"
#include "UObject/UObjectIterator.h"
#include <algorithm>

#define LOCTEXT_NAMESPACE "TrueSkyWaterComponent"
//#define LOCTEXT_NAMESPACE "TrueSkyWaterComponent"
extern ENGINE_API int32 GReflectionCaptureSize;

TArray<UTrueSkyWaterComponent*> UTrueSkyWaterComponent::TrueSkyWaterComponents;

UTrueSkyWaterComponent::UTrueSkyWaterComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UTrueSkyWaterComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags,
	ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
	AActor* waterActor = GetAttachmentRootActor();
	waterActor->PostInitializeComponents();
}

#undef LOCTEXT_NAMESPACE