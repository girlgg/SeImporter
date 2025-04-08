#include "GameInfo/ModernWarfare6.h"

#include "WraithX/CoDAssetType.h"
#include "WraithX/GameProcess.h"

FModernWarfare6::FModernWarfare6(FCordycepProcess* InProcess)
	: TBaseGame(InProcess)
{
	GFXMAP_POOL_IDX = 50;
}

TArray<FCastTextureInfo> FModernWarfare6::PopulateMaterial(const FMW6Material& Material)
{
	TArray<FMW6GfxImage> Images;
	Images.Reserve(Material.ImageCount);

	for (uint32 i = 0; i < Material.ImageCount; i++)
	{
		const uint64 ImagePtr = Process->ReadMemory<uint64>(Material.ImageTable + i * 8);
		FMW6GfxImage Image = Process->ReadMemory<FMW6GfxImage>(ImagePtr);
		Images.Add(Image);
	}

	TArray<FCastTextureInfo> Textures;

	for (int i = 0; i < Material.TextureCount; i++)
	{
		FMW6MaterialTextureDef TextureDef = Process->ReadMemory<FMW6MaterialTextureDef>(
			Material.TextureTable + i * sizeof(FMW6MaterialTextureDef));
		FMW6GfxImage Image = Images[TextureDef.ImageIdx];

		int UVMapIndex = 0;

		uint64 Hash = Image.Hash & 0x0FFFFFFFFFFFFFFF;

		FString ImageName = FString::Printf(TEXT("ximage_%llx"), Hash);

		FString TextureSemantic = FString::Printf(TEXT("unk_semantic_0x%x"), TextureDef.Index);

		FCastTextureInfo TextureInfo;
		TextureInfo.TextureName = ImageName;
		TextureInfo.TextureSemantic = TextureSemantic;
		Textures.Add(TextureInfo);
	}

	return Textures;
}

float FModernWarfare6::GetSurfaceScale(const FMW6XSurface& Surface)
{
	return Surface.OverrideScale != -1 ? Surface.OverrideScale : FMath::Max3(Surface.Min, Surface.Scale, Surface.Max);
}

FVector3f FModernWarfare6::GetSurfaceOffset(const FMW6XSurface& Surface)
{
	return Surface.OverrideScale != -1
		       ? FVector3f::Zero()
		       : FVector3f(Surface.OffsetsX, Surface.OffsetsY, Surface.OffsetsZ);
}

uint64 FModernWarfare6::GetXPakKey(const FMW6XModelSurfs& XModelSurfs, const FMW6XSurfaceShared& Shared)
{
	return XModelSurfs.XPakKey;
}

uint64 FModernWarfare6::GetXyzOffset(const FMW6XSurface& Surface)
{
	return Surface.XyzOffset;
}

uint64 FModernWarfare6::GetTangentFrameOffset(const FMW6XSurface& Surface)
{
	return Surface.TangentFrameOffset;
}

uint64 FModernWarfare6::GetTexCoordOffset(const FMW6XSurface& Surface)
{
	return Surface.TexCoordOffset;
}

uint64 FModernWarfare6::GetColorOffset(const FMW6XSurface& Surface)
{
	return Surface.ColorOffset;
}

uint64 FModernWarfare6::GetIndexDataOffset(const FMW6XSurface& Surface)
{
	return Surface.IndexDataOffset;
}

uint64 FModernWarfare6::GetPackedIndicesOffset(const FMW6XSurface& Surface)
{
	return Surface.PackedIndicesOffset;
}

uint8 FModernWarfare6::ReadXImage(TSharedPtr<FGameProcess> ProcessInstance,
                                  TSharedPtr<FCoDImage> ImageAddr, TArray<uint8>& ImageDataArray)
{
	FMW6GfxImage ImageInfo = ProcessInstance->ReadMemory<FMW6GfxImage>(ImageAddr->AssetPointer);

	if (ImageInfo.LoadedImagePtr)
	{
		uint32 ImageSize = ImageInfo.BufferSize;
		ImageDataArray.SetNumUninitialized(ImageSize);

		uint64 ReadSize =
			ProcessInstance->ReadArray(ImageInfo.LoadedImagePtr, ImageDataArray, ImageInfo.BufferSize);
		if (ReadSize != ImageSize)
		{
			ImageDataArray.Empty();
		}
	}
	return ImageInfo.ImageFormat;
}

void FModernWarfare6::ReadXModel(TSharedPtr<FGameProcess> ProcessInstance, TSharedPtr<FCoDModel> ModelAddr)
{
	FWraithXModel ModelAsset;
	FMW6XModel ModelData = ProcessInstance->ReadMemory<FMW6XModel>(ModelAddr->AssetPointer);

	ModelAsset.ModelName = ModelAddr->AssetName;

	uint32 BoneCount = 0;

	if (ModelData.NumBones + ModelData.UnkBoneCount > 1 && ModelData.ParentListPtr == 0)
	{
		ModelAsset.BoneCount = 0;
		ModelAsset.RootBoneCount = 0;
	}
	else
	{
		BoneCount = ModelAsset.BoneCount = ModelData.NumBones + ModelData.UnkBoneCount;
		ModelAsset.RootBoneCount = ModelData.NumRootBones;
	}
	ModelAsset.BoneRotationData = EBoneDataTypes::DivideBySize;
	ModelAsset.IsModelStreamed = true;
	ModelAsset.BoneIDsPtr = ModelData.BoneIdsPtr;
	ModelAsset.BoneIndexSize = 4;
	ModelAsset.BoneParentsPtr = ModelData.ParentListPtr;
	ModelAsset.BoneParentSize = 2;
	ModelAsset.RotationsPtr = ModelData.RotationsPtr;
	ModelAsset.TranslationsPtr = ModelData.TranslationsPtr;
	ModelAsset.BaseMatriciesPtr = ModelData.BaseMaterialsPtr;

	// 只读取LOD0
	FMW6XModelLod ModelLod = ProcessInstance->ReadMemory<FMW6XModelLod>(ModelData.LodInfo);

	FWraithXModelLod& LodReference = ModelAsset.ModelLods.AddDefaulted_GetRef();

	LodReference.LODStreamInfoPtr = ModelLod.MeshPtr;

	FMW6XModelSurfs XModelSurfs = ProcessInstance->ReadMemory<FMW6XModelSurfs>(ModelLod.MeshPtr);
	uint64 MaterialHandle = ProcessInstance->ReadMemory<uint64>(ModelData.MaterialHandles);

	for (uint32 SurfaceIdx = 0; SurfaceIdx < ModelLod.NumSurfs; ++SurfaceIdx)
	{
		FWraithXModelSubmesh& SubmeshRef = LodReference.Submeshes.AddDefaulted_GetRef();

		FMW6XSurface Surface = ProcessInstance->ReadMemory<FMW6XSurface>(
			XModelSurfs.Surfs + SurfaceIdx * sizeof(FMW6XSurface));

		// SubmeshRef.VertListcount = Surface.VertListcount;

		FMW6Material Material = ProcessInstance->ReadMemory<FMW6Material>(
			MaterialHandle + SurfaceIdx * sizeof(FMW6Material));

		// LodReference.Materials.Add();

		FCastMeshInfo MeshInfo;
	}

	FCastModelInfo ModelResult;
	// FMW6XModelM
	FMW6XModelSurfs MeshInfo = ProcessInstance->ReadMemory<FMW6XModelSurfs>(ModelLod.MeshPtr);
	FMW6XSurfaceShared BufferInfo = ProcessInstance->ReadMemory<FMW6XSurfaceShared>(
		MeshInfo.Shared);

	TArray<uint8> MeshDataBuffer;
	if (BufferInfo.Data)
	{
		ProcessInstance->ReadArray(BufferInfo.Data, MeshDataBuffer, BufferInfo.DataSize);
	}
	else
	{
		MeshDataBuffer = ProcessInstance->GetDecrypt()->ExtractXSubPackage(MeshInfo.XPakKey, BufferInfo.DataSize);
	}
	// 获取骨骼信息
	if (BoneCount > 1)
	{
		FCastSkeletonInfo& SkeletonInfo = ModelResult.Skeletons.AddDefaulted_GetRef();
		for (uint32 BoneIdx = 0; BoneIdx < BoneCount; ++BoneIdx)
		{
			FCastBoneInfo& Bone = SkeletonInfo.Bones.AddDefaulted_GetRef();

			uint32 BoneNameHash = ProcessInstance->ReadMemory<uint32>(ModelData.BoneIdsPtr + BoneIdx * 4);
			FString BoneName;
			if (!FCoDAssetDatabase::Get().AssetName_QueryValue(BoneNameHash, BoneName))
			{
				BoneName = FString::Printf(TEXT("bone_%u"), BoneNameHash);
			}
			if (BoneName.IsEmpty())
			{
				if (BoneIdx)
				{
					BoneName = FString::Printf(TEXT("no_tag_%u"), BoneIdx);
				}
				else
				{
					BoneName = TEXT("tag_origin");
				}
			}
		}
	}
}
