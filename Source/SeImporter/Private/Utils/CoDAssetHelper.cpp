﻿#include "Utils/CoDAssetHelper.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

UTexture2D* FCoDAssetHelper::CreateTextureFromDDSData(const TArray<uint8>& ImageDataArray, int32 Width, int32 Height,
                                                      uint8 ImageFormat, const FString& TextureName)
{
	// SNORM 有符号归一化，范围是-1.0到1.0；UNORM 无符号归一化，范围是0.0到1.0。
	UTexture2D* NewTexture = NewObject<UTexture2D>(GetTransientPackage(), FName(*TextureName), RF_Transient);
	FTexturePlatformData* PlatformData = new FTexturePlatformData();
	if (!NewTexture || !PlatformData)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create texture"));
		return nullptr;
	}
	switch (ImageFormat)
	{
	// --- 标准未压缩格式 ---
	case 24: // DDS_Standard_R10G10B10A2_UNORM
		PlatformData->PixelFormat = PF_A2B10G10R10;
		break;
	case 28: // DDS_Standard_A8R8G8B8
		PlatformData->PixelFormat = PF_B8G8R8A8;
		break;
	case 29: // DDS_Standard_A8R8G8B8_SRGB
		NewTexture->SRGB = true;
		PlatformData->PixelFormat = PF_B8G8R8A8;
		break;
	case 61: // DDS_Standard_R8_UNORM
		PlatformData->PixelFormat = PF_R8;
		break;
	case 62: // DDS_Standard_R8_UINT
		NewTexture->CompressionSettings = TC_Displacementmap;
		PlatformData->PixelFormat = PF_R8;
		break;
	// --- BC压缩格式 ---
	case 71: // DDS_BC1_UNORM
		PlatformData->PixelFormat = PF_DXT1;
		break;
	case 72: // DDS_BC1_SRGB
		NewTexture->SRGB = true;
		PlatformData->PixelFormat = PF_DXT1;
		break;
	case 74: // DDS_BC2_UNORM
		PlatformData->PixelFormat = PF_DXT3;
		break;
	case 75: // DDS_BC2_SRGB
		NewTexture->SRGB = true;
		PlatformData->PixelFormat = PF_DXT3;
		break;
	case 77: // DDS_BC3_UNORM
		PlatformData->PixelFormat = PF_DXT5;
		break;
	case 78: // DDS_BC3_SRGB
		NewTexture->SRGB = true;
		PlatformData->PixelFormat = PF_DXT5;
		break;
	case 80: // DDS_BC4_UNORM
		PlatformData->PixelFormat = PF_BC4;
		break;
	case 81: // DDS_BC4_SNORM
		NewTexture->CompressionSettings = TC_Normalmap;
		PlatformData->PixelFormat = PF_BC4;
		break;
	case 83: // DDS_BC5_UNORM
		PlatformData->PixelFormat = PF_BC5;
		break;
	case 84: // DDS_BC5_SNORM
		NewTexture->CompressionSettings = TC_Normalmap;
		NewTexture->LODGroup = TEXTUREGROUP_WorldNormalMap;
		PlatformData->PixelFormat = PF_BC5;
		break;
	case 95: // DDS_BC6_UF16
	case 96: // DDS_BC6_SF16
		NewTexture->CompressionSettings = TC_HDR; // HDR纹理
		PlatformData->PixelFormat = PF_BC6H;
		break;
	case 98: // DDS_BC7_UNORM
		PlatformData->PixelFormat = PF_BC7;
		break;
	case 99: // DDS_BC7_SRGB
		NewTexture->SRGB = true;
		PlatformData->PixelFormat = PF_BC7;
		break;
	default:
		PlatformData->PixelFormat = PF_Unknown;
		break;
	}

	NewTexture->Filter = TF_Default;

	PlatformData->SizeX = Width;
	PlatformData->SizeY = Height;

	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	PlatformData->Mips.Add(Mip);
	Mip->SizeX = Width;
	Mip->SizeY = Height;

	Mip->BulkData.Lock(LOCK_READ_WRITE);
	void* TextureData = Mip->BulkData.Realloc(ImageDataArray.Num());
	FMemory::Memcpy(TextureData, ImageDataArray.GetData(), ImageDataArray.Num());
	Mip->BulkData.Unlock();

	NewTexture->SetPlatformData(PlatformData);

	NewTexture->UpdateResource();
	return NewTexture;
}

bool FCoDAssetHelper::SaveObjectToPackage(UObject* ObjectToSave, const FString& PackagePath)
{
	if (!ObjectToSave || PackagePath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid object or package path"));
		return false;
	}

	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackagePath);
		return false;
	}

	FString ObjectName = FPaths::GetBaseFilename(PackagePath);
	ObjectToSave->Rename(*ObjectName, Package, REN_NonTransactional | REN_DontCreateRedirectors);

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(ObjectToSave);

	FString FilePath = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bForceByteSwapping = false;
	SaveArgs.bWarnOfLongFilename = true;
	SaveArgs.SaveFlags = SAVE_None;
	SaveArgs.bSlowTask = true;

	if (UPackage::SavePackage(Package, ObjectToSave, *FilePath, SaveArgs))
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully saved: %s"), *FilePath);
		return true;
	}
	UE_LOG(LogTemp, Error, TEXT("Failed to save package: %s"), *FilePath);
	return false;
}
