#include "WraithX/WraithX.h"

#include "GameInfo/ModernWarfare6.h"
#include "Utils/CoDAssetHelper.h"
#include "WraithX/GameProcess.h"

void FWraithX::InitializeGame()
{
	if (ProcessInstance.IsValid())
	{
		ProcessInstance.Reset();
	}
	ProcessInstance = MakeShared<FGameProcess>();
	ProcessInstance->OnOnAssetLoadingDelegate.AddRaw(this, &FWraithX::OnAssetLoadingCall);
	FCoDAssetDatabase::Get().GetTaskCompletedHandle().AddRaw(this, &FWraithX::OnAssetInitCompletedCall);
}

void FWraithX::RefreshGame()
{
	if (ProcessInstance)
	{
		ProcessInstance->LoadGameFromParasyte();
		FCoDAssetDatabase::Get().GetTaskCompletedHandle().AddRaw(this, &FWraithX::OnAssetInitCompletedCall);
	}
}

void FWraithX::ImportModel(FString ImportPath, TSharedPtr<FCoDAsset> Asset)
{
	TSharedPtr<FCoDModel> ModelInfo = StaticCastSharedPtr<FCoDModel>(Asset);
	switch (ProcessInstance->GetCurrentGameType())
	{
	case CoDAssets::ESupportedGames::ModernWarfare5:
		break;
	case CoDAssets::ESupportedGames::ModernWarfare6:
		FModernWarfare6::ReadXModel(ProcessInstance, ModelInfo);
		break;
	default:
		break;
	}
}

void FWraithX::ImportImage(FString ImportPath, TSharedPtr<FCoDAsset> Asset)
{
	TSharedPtr<FCoDImage> Image = StaticCastSharedPtr<FCoDImage>(Asset);
	TArray<uint8> ImageData;
	uint8 ImageFormat = 0;

	if (Image->bIsFileEntry)
	{
		switch (ProcessInstance->GetCurrentGameType())
		{
		case CoDAssets::ESupportedGames::WorldAtWar:
		case CoDAssets::ESupportedGames::ModernWarfare:
		case CoDAssets::ESupportedGames::ModernWarfare2:
		case CoDAssets::ESupportedGames::ModernWarfare3:
		case CoDAssets::ESupportedGames::BlackOps:
			break;
		case CoDAssets::ESupportedGames::BlackOps2:
			break;
		case CoDAssets::ESupportedGames::BlackOps3:
		case CoDAssets::ESupportedGames::BlackOps4:
		case CoDAssets::ESupportedGames::BlackOpsCW:
		case CoDAssets::ESupportedGames::ModernWarfare4:
		case CoDAssets::ESupportedGames::ModernWarfare5:
			break;
		default:
			break;
		}
	}
	else
	{
		switch (ProcessInstance->GetCurrentGameType())
		{
		case CoDAssets::ESupportedGames::BlackOps3:
			break;
		case CoDAssets::ESupportedGames::BlackOps4:
			break;
		case CoDAssets::ESupportedGames::BlackOpsCW:
			break;
		case CoDAssets::ESupportedGames::Ghosts:
			break;
		case CoDAssets::ESupportedGames::AdvancedWarfare:
			break;
		case CoDAssets::ESupportedGames::ModernWarfareRemastered:
			break;
		case CoDAssets::ESupportedGames::ModernWarfare2Remastered:
			break;
		case CoDAssets::ESupportedGames::ModernWarfare4:
			break;
		case CoDAssets::ESupportedGames::ModernWarfare5:
			break;
		case CoDAssets::ESupportedGames::ModernWarfare6:
			ImageFormat = FModernWarfare6::ReadXImage(ProcessInstance, Image, ImageData);
		// FModernWarfare6
			break;
		case CoDAssets::ESupportedGames::WorldWar2:
			break;
		case CoDAssets::ESupportedGames::Vanguard:
			break;
		default:
			break;
		}
	}
	if (!ImageData.IsEmpty())
	{
		UTexture2D* Texture = FCoDAssetHelper::CreateTextureFromDDSData(ImageData,
		                                                                Image->Width,
		                                                                Image->Height,
		                                                                ImageFormat,
		                                                                Image->AssetName);
		// FCoDAssetHelper::SaveObjectToPackage(Texture, Image->AssetName);
	}
}

void FWraithX::ImportSelection(FString ImportPath, TArray<TSharedPtr<FCoDAsset>> Selection)
{
	for (auto& Asset : Selection)
	{
		switch (Asset->AssetType)
		{
		case EWraithAssetType::Animation:
			break;
		case EWraithAssetType::Image:
			ImportImage(ImportPath, Asset);
			break;
		case EWraithAssetType::Model:
			ImportModel(ImportPath, Asset);
			break;
		case EWraithAssetType::Sound:
			break;
		case EWraithAssetType::RawFile:
			break;
		case EWraithAssetType::Material:
			break;
		default:
			break;
		}
	}
}

void FWraithX::OnAssetInitCompletedCall()
{
	OnOnAssetInitCompletedDelegate.Broadcast();
	OnOnAssetInitCompletedDelegate.Clear();
}

void FWraithX::OnAssetLoadingCall(float InProgress)
{
	OnOnAssetLoadingDelegate.Broadcast(InProgress);
}
