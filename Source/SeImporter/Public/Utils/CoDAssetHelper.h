#pragma once

class FCoDAssetHelper
{
public:
	static UTexture2D* CreateTextureFromDDSData(const TArray<uint8>& ImageDataArray, int32 Width, int32 Height,
	                                            uint8 ImageFormat, const FString& TextureName);
	static bool SaveObjectToPackage(UObject* ObjectToSave, const FString& PackagePath);
};
