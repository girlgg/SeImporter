#pragma once

#include "CoreMinimal.h"
#include "CoDAssetType.h"
#include "GameProcess.h"

struct FCastAnimationInfo;
struct FCastModelInfo;
DECLARE_MULTICAST_DELEGATE(FOnOnAssetInitCompletedDelegate);

class FGameProcess;

class FWraithX
{
public:
	void InitializeGame();
	void RefreshGame();

	FORCEINLINE TArray<TSharedPtr<FCoDAsset>>& GetLoadedAssets() const { return ProcessInstance->GetLoadedAssets(); }

	void ImportAnim(FString ImportPath, TSharedPtr<FCoDAsset> Asset);
	void ImportModel(FString ImportPath, TSharedPtr<FCoDAsset> Asset);
	void ImportImage(FString ImportPath, TSharedPtr<FCoDAsset> Asset);
	void ImportSound(FString ImportPath,TSharedPtr<FCoDAsset> Asset);
	void ImportMaterial(FString ImportPath,TSharedPtr<FCoDAsset> Asset);
	void ImportSelection(FString ImportPath, TArray<TSharedPtr<FCoDAsset>> Selection);

	void LoadGenericAnimAsset(FWraithXAnim& OutAnim, TSharedPtr<FCoDAnim> AnimInfo);

	void LoadGenericModelAsset(FWraithXModel& OutModel, TSharedPtr<FCoDModel> ModelInfo);
	void TranslateXModel(FCastModelInfo& OutModel, FWraithXModel& InModel, int32 LodIdx);

	void TranslateXAnim(FCastAnimationInfo& OutAnim, FWraithXAnim& InAnim);
	void DeltaTranslation64(FCastAnimationInfo& OutAnim, FWraithXAnim& InAnim, uint32 FrameSize);
	void Delta2DRotation64(FCastAnimationInfo& OutAnim, FWraithXAnim& InAnim, uint32 FrameSize);
	void Delta3DRotation64(FCastAnimationInfo& OutAnim, FWraithXAnim& InAnim, uint32 FrameSize);

	FOnOnAssetInitCompletedDelegate OnOnAssetInitCompletedDelegate;
	FOnOnAssetLoadingDelegate OnOnAssetLoadingDelegate;

protected:
	void OnAssetInitCompletedCall();
	void OnAssetLoadingCall(float InProgress);

private:
	TSharedPtr<FGameProcess> ProcessInstance;
};
