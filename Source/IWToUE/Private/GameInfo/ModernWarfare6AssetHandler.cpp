#include "GameInfo/ModernWarfare6AssetHandler.h"

bool FModernWarfare6AssetHandler::ReadModelData(TSharedPtr<FCoDModel> ModelInfo, FWraithXModel& OutModel)
{
	return false;
}

bool FModernWarfare6AssetHandler::ReadAnimData(TSharedPtr<FCoDAnim> AnimInfo, FWraithXAnim& OutAnim)
{
	return false;
}

bool FModernWarfare6AssetHandler::ReadImageData(TSharedPtr<FCoDImage> ImageInfo, TArray<uint8>& OutImageData,
                                                uint8& OutFormat)
{
	return false;
}

bool FModernWarfare6AssetHandler::ReadSoundData(TSharedPtr<FCoDSound> SoundInfo, FWraithXSound& OutSound)
{
	return false;
}

bool FModernWarfare6AssetHandler::ReadMaterialData(TSharedPtr<FCoDMaterial> MaterialInfo, FWraithXMaterial& OutMaterial)
{
	return false;
}

bool FModernWarfare6AssetHandler::TranslateModel(const FWraithXModel& InModel, int32 LodIdx,
                                                 FCastModelInfo& OutModelInfo)
{
	return false;
}

bool FModernWarfare6AssetHandler::TranslateAnim(const FWraithXAnim& InAnim, FCastAnimationInfo& OutAnimInfo)
{
	return false;
}

void FModernWarfare6AssetHandler::ApplyDeltaTranslation(FCastAnimationInfo& OutAnim, const FWraithXAnim& InAnim)
{
}

void FModernWarfare6AssetHandler::ApplyDelta2DRotation(FCastAnimationInfo& OutAnim, const FWraithXAnim& InAnim)
{
}

void FModernWarfare6AssetHandler::ApplyDelta3DRotation(FCastAnimationInfo& OutAnim, const FWraithXAnim& InAnim)
{
}

bool FModernWarfare6AssetHandler::LoadStreamedModelData(const FWraithXModel& InModel, FWraithXModelLod& InOutLod,
                                                        FCastModelInfo& OutModelInfo)
{
	return false;
}

bool FModernWarfare6AssetHandler::LoadStreamedAnimData(const FWraithXAnim& InAnim, FCastAnimationInfo& OutAnimInfo)
{
	return false;
}
