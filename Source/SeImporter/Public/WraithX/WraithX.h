#pragma once

#include "CoreMinimal.h"
#include "CoDAssetType.h"
#include "GameProcess.h"

struct FCastModelInfo;
DECLARE_MULTICAST_DELEGATE(FOnOnAssetInitCompletedDelegate);

class FGameProcess;

class FWraithX
{
public:
	void InitializeGame();
	void RefreshGame();

	FORCEINLINE TArray<TSharedPtr<FCoDAsset>>& GetLoadedAssets() const { return ProcessInstance->GetLoadedAssets(); }

	// void ImportAnim(TSharedPtr<FCoDAsset> Asset);
	void ImportModel(FString ImportPath, TSharedPtr<FCoDAsset> Asset);
	void ImportImage(FString ImportPath, TSharedPtr<FCoDAsset> Asset);
	// void ImportSound(TSharedPtr<FCoDAsset> Asset);
	// void ImportMaterial(TSharedPtr<FCoDAsset> Asset);
	// void ImportRawFile(TSharedPtr<FCoDAsset> Asset);
	void ImportSelection(FString ImportPath, TArray<TSharedPtr<FCoDAsset>> Selection);

	void LoadGenericModelAsset(FWraithXModel& OutModel, TSharedPtr<FCoDModel> ModelInfo);
	void TranslateXModel(FCastModelInfo& OutModel, FWraithXModel& InModel, int32 LodIdx);

	FOnOnAssetInitCompletedDelegate OnOnAssetInitCompletedDelegate;
	FOnOnAssetLoadingDelegate OnOnAssetLoadingDelegate;
protected:
	void OnAssetInitCompletedCall();
	void OnAssetLoadingCall(float InProgress);

private:
	TSharedPtr<FGameProcess> ProcessInstance;
};
