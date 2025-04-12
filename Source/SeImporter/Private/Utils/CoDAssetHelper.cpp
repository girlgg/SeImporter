#include "Utils/CoDAssetHelper.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CastManager/CastModel.h"
#include "UObject/SavePackage.h"
#include "WraithX/CoDAssetType.h"
#include "WraithX/GameProcess.h"

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

uint8 FCoDMeshHelper::FindFaceIndex(TSharedPtr<FGameProcess> ProcessInstance, uint64 PackedIndices, uint32 Index,
                                    uint8 Bits, bool IsLocal)
{
	unsigned long BitIndex;

	if (!_BitScanReverse64(&BitIndex, Bits)) BitIndex = 64;
	else BitIndex ^= 0x3F;

	const uint16 Offset = Index * (64 - BitIndex);
	const uint8 BitCount = 64 - BitIndex;
	const uint64 PackedIndicesPtr = PackedIndices + (Offset >> 3);
	const uint8 BitOffset = Offset & 7;

	const uint8 PackedIndice = ProcessInstance->ReadMemory<uint8>(PackedIndicesPtr, IsLocal);

	if (BitOffset == 0)
		return PackedIndice & ((1 << BitCount) - 1);

	if (8 - BitOffset < BitCount)
	{
		const uint8 nextPackedIndice = ProcessInstance->ReadMemory<uint8>(PackedIndicesPtr + 1, IsLocal);
		return (PackedIndice >> BitOffset) & ((1 << (8 - BitOffset)) - 1) | ((nextPackedIndice & ((1 << (64 -
			BitIndex - (8 - BitOffset))) - 1)) << (8 - BitOffset));
	}

	return (PackedIndice >> BitOffset) & ((1 << BitCount) - 1);
}

bool FCoDMeshHelper::UnpackFaceIndices(TSharedPtr<FGameProcess> ProcessInstance, TArray<uint16>& InFacesArr,
                                       uint64 Tables, uint64 TableCount, uint64 PackedIndices, uint64 Indices,
                                       uint64 FaceIndex, const bool IsLocal)
{
	InFacesArr.SetNum(3);
	uint64 CurrentFaceIndex = FaceIndex;
	for (int i = 0; i < TableCount; i++)
	{
		uint64 TablePtr = Tables + (i * 40);
		uint64 IndicesPtr = PackedIndices + ProcessInstance->ReadMemory<uint32>(TablePtr + 36, IsLocal);
		uint8 Count = ProcessInstance->ReadMemory<uint8>(TablePtr + 35, IsLocal);
		if (CurrentFaceIndex < Count)
		{
			uint8 Bits = (uint8)(ProcessInstance->ReadMemory<uint8>(TablePtr + 34, IsLocal) - 1);
			FaceIndex = ProcessInstance->ReadMemory<uint32>(TablePtr + 28, IsLocal);

			uint32 Face1Offset = FindFaceIndex(ProcessInstance, IndicesPtr, CurrentFaceIndex * 3 + 0, Bits, IsLocal) +
				FaceIndex;
			uint32 Face2Offset = FindFaceIndex(ProcessInstance, IndicesPtr, CurrentFaceIndex * 3 + 1, Bits, IsLocal) +
				FaceIndex;
			uint32 Face3Offset = FindFaceIndex(ProcessInstance, IndicesPtr, CurrentFaceIndex * 3 + 2, Bits, IsLocal) +
				FaceIndex;

			InFacesArr[0] = ProcessInstance->ReadMemory<uint16>(Indices + Face1Offset * 2, IsLocal);
			InFacesArr[1] = ProcessInstance->ReadMemory<uint16>(Indices + Face2Offset * 2, IsLocal);
			InFacesArr[2] = ProcessInstance->ReadMemory<uint16>(Indices + Face3Offset * 2, IsLocal);
			return true;
		}
		CurrentFaceIndex -= Count;
	}
	return false;
}
