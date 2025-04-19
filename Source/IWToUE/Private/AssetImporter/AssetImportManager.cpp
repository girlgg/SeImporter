#include "AssetImporter/AssetImportManager.h"

#include "AssetImporter/AnimationImporter.h"
#include "AssetImporter/ImageImporter.h"
#include "AssetImporter/MaterialImporter.h"
#include "AssetImporter/ModelImporter.h"
#include "AssetImporter/SoundImporter.h"
#include "Database/CoDDatabaseService.h"
#include "GameInfo/GameAssetHandlerFactory.h"
#include "WraithX/CoDAssetType.h"
#include "WraithX/GameProcess.h"

FAssetImportManager::FAssetImportManager()
{
	SetupAssetImporters();
}

void FAssetImportManager::InitializeGame()
{
	if (ProcessInstance.IsValid())
	{
		ProcessInstance->OnAssetLoadingProgress.RemoveAll(this);
	}
	else
	{
		ProcessInstance = MakeShared<FGameProcess>();
	}
	ProcessInstance->Initialize();
	ProcessInstance->StartAssetDiscovery();
	ProcessInstance->OnAssetLoadingProgress.AddRaw(this, &FAssetImportManager::OnAssetLoadingInternal);
	ProcessInstance->OnAssetLoadingComplete.AddRaw(this, &FAssetImportManager::OnAssetInitCompletedInternal);
	UpdateGameHandler();
	UE_LOG(LogTemp, Log, TEXT("Game Initialized. Handler created."));
}

void FAssetImportManager::RefreshGame()
{
	if (ProcessInstance.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Refreshing game assets..."));
		// Re-bind completion handle just in case (depends on FCoDAssetDatabase behavior)
		// FCoDAssetDatabase::Get().GetTaskCompletedHandle().RemoveAll(this); // Clean up previous binding
		// FCoDAssetDatabase::Get().GetTaskCompletedHandle().AddRaw(this, &FAssetImportManager::OnAssetInitCompletedInternal);

		// Update the game handler in case the game type changed (unlikely?) or needs reset
		UpdateGameHandler();

		// Trigger the asset list loading process
		// ProcessInstance->LoadGameFromParasyte();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot refresh game, ProcessInstance is not valid."));
		InitializeGame();
	}
}

TArray<TSharedPtr<FCoDAsset>> FAssetImportManager::GetLoadedAssets() const
{
	if (ProcessInstance)
	{
		return ProcessInstance->GetLoadedAssets();
	}
	return TArray<TSharedPtr<FCoDAsset>>();
}

void FAssetImportManager::ImportSelection(FString ImportPath, TArray<TSharedPtr<FCoDAsset>> Selection, FString OptionalParams)
{
}

void FAssetImportManager::OnAssetInitCompletedInternal()
{
	OnAssetInitCompletedDelegate.Broadcast();
}

void FAssetImportManager::OnAssetLoadingInternal(float InProgress)
{
	OnAssetLoadingDelegate.Broadcast(InProgress);
}

void FAssetImportManager::SetupAssetImporters()
{
	AssetImporters.Emplace(EWraithAssetType::Animation, MakeShared<FAnimationImporter>());
	AssetImporters.Emplace(EWraithAssetType::Image, MakeShared<FImageImporter>());
	AssetImporters.Emplace(EWraithAssetType::Model, MakeShared<FModelImporter>());
	AssetImporters.Emplace(EWraithAssetType::Sound, MakeShared<FSoundImporter>());
	AssetImporters.Emplace(EWraithAssetType::Material, MakeShared<FMaterialImporter>());
}

void FAssetImportManager::UpdateGameHandler()
{
	CurrentGameHandler = FGameAssetHandlerFactory::CreateHandler(ProcessInstance);
	if (!CurrentGameHandler.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create a valid game asset handler!"));
	}
}
