#pragma once

#include "CoreMinimal.h"
#include "CoDAssetType.h"
#include "GameProcess.h"

DECLARE_MULTICAST_DELEGATE(FOnOnAssetInitCompletedDelegate);

class FGameProcess;

class FWraithX
{
public:
	void InitializeGame();

	FORCEINLINE TArray<TSharedPtr<FCoDAsset>>& GetLoadedAssets() const { return ProcessInstance->GetLoadedAssets(); }

	FOnOnAssetInitCompletedDelegate OnOnAssetInitCompletedDelegate;
protected:
	void OnAssetInitCompletedCall();

private:
	TUniquePtr<FGameProcess> ProcessInstance;
};
