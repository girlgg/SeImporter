#include "WraithX/WraithX.h"

#include "CastManager/CastRoot.h"
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
	FWraithXModel GenericModel;
	LoadGenericModelAsset(GenericModel, ModelInfo);

	// 准备材质
	for (FWraithXModelLod& LodModel : GenericModel.ModelLods)
	{
		for (FWraithXMaterial& Material : LodModel.Materials)
		{
			// 导出材质图像名
			// 导出材质图像
			// 注意重复性校验
		}
	}
	// 准备模型
	const int32 LodCount = GenericModel.ModelLods.Num();

	FCastRoot SceneRoot;
	SceneRoot.Models.Reserve(LodCount);
	SceneRoot.ModelLodInfo.Reserve(LodCount);

	for (int32 LodIdx = 0; LodIdx < LodCount; ++LodIdx)
	{
		FCastModelInfo ModelResult;

		TranslateXModel(ModelResult, GenericModel, LodIdx);

		FCastModelLod ModelLod;
		ModelLod.Distance = GenericModel.ModelLods[LodIdx].LodDistance;
		ModelLod.MaxDistance = GenericModel.ModelLods[LodIdx].LodMaxDistance;

		ModelLod.ModelIndex = SceneRoot.Models.Add(ModelResult);
		SceneRoot.ModelLodInfo.Add(ModelLod);
	}
	// 导出模型
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

void FWraithX::LoadGenericModelAsset(FWraithXModel& OutModel, TSharedPtr<FCoDModel> ModelInfo)
{
	switch (ProcessInstance->GetCurrentGameType())
	{
	case CoDAssets::ESupportedGames::ModernWarfare6:
		FModernWarfare6::ReadXModel(OutModel, ProcessInstance, ModelInfo);
		break;
	default:
		break;
	}
}

void FWraithX::TranslateXModel(FCastModelInfo& OutModel, FWraithXModel& InModel, int32 LodIdx)
{
	OutModel.Name = InModel.ModelName;

	FWraithXModelLod& LodRef = InModel.ModelLods[LodIdx];

	TMap<FString, uint32> MaterialMap;

	OutModel.Materials.Reserve(LodRef.Materials.Num());
	for (int32 MatIdx = 0; MatIdx < LodRef.Materials.Num(); ++MatIdx)
	{
		FWraithXMaterial& MatRef = LodRef.Materials[MatIdx];

		if (MaterialMap.Contains(MatRef.MaterialName))
		{
			LodRef.Submeshes[MatIdx].MaterialIndex = MaterialMap[MatRef.MaterialName];
		}
		else
		{
			FCastMaterialInfo& MaterialInfo = OutModel.Materials.AddDefaulted_GetRef();
			MaterialInfo.Name = MatRef.MaterialName;

			// TODO 加载材质
			for (const FWraithXImage& Image : MatRef.Images)
			{
				FCastTextureInfo& Texture = MaterialInfo.Textures.AddDefaulted_GetRef();
				Texture.TextureName = Image.ImageName;

				FString TextureSemantic = FString::Printf(TEXT("unk_semantic_0x%x"), Image.SemanticHash);
				Texture.TextureSemantic = TextureSemantic;
			}
			MaterialMap.Emplace(MatRef.MaterialName, OutModel.Materials.Num() - 1);
		}
	}
	if (InModel.IsModelStreamed)
	{
		switch (ProcessInstance->GetCurrentGameType())
		{
		case CoDAssets::ESupportedGames::ModernWarfare6:
			FModernWarfare6::LoadXModel(ProcessInstance, InModel, LodRef, OutModel);
			break;
		default:
			break;
		}
	}
	else
	{
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
