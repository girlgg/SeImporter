#include "GameInfo/ModernWarfare6SP.h"

FModernWarfare6SP::FModernWarfare6SP(FCordycepProcess* InProcess)
	: TBaseGame(InProcess)
{
	GFXMAP_POOL_IDX = 50;
}

TArray<FCastTextureInfo> FModernWarfare6SP::PopulateMaterial(const FMW6SPMaterial& Material)
{
	TArray<FCastTextureInfo> Textures;

	for (int i = 0; i < Material.TextureCount; i++)
	{
		FMW6SPMaterialTextureDef TextureDef = Process->ReadMemory<FMW6SPMaterialTextureDef>(
			Material.TextureTable + i * sizeof(FMW6SPMaterialTextureDef));
		FMW6GfxImage Image = Process->ReadMemory<FMW6GfxImage>(TextureDef.ImagePtr);

		int UVMapIndex = 0;

		uint64 Hash = Image.Hash & 0x0FFFFFFFFFFFFFFF;

		FString ImageName = FString::Printf(TEXT("ximage_%llX"), Hash);

		FString TextureSemantic = FString::Printf(TEXT("unk_semantic_0x%x"), TextureDef.Index);

		FCastTextureInfo TextureInfo;
		TextureInfo.TextureName = ImageName;
	}

	return Textures;
}

FVector3f FModernWarfare6SP::GetSurfaceOffset(const FMW6XSurface& Surface)
{
	return Surface.OverrideScale != -1
		       ? FVector3f::Zero()
		       : FVector3f(Surface.OffsetsX, Surface.OffsetsY, Surface.OffsetsZ);
}

float FModernWarfare6SP::GetSurfaceScale(const FMW6XSurface& Surface)
{
	return Surface.OverrideScale != -1 ? Surface.OverrideScale : FMath::Max3(Surface.Min, Surface.Scale, Surface.Max);
}

uint64 FModernWarfare6SP::GetXPakKey(const FMW6XModelSurfs& XModelSurfs, const FMW6XSurfaceShared& Shared)
{
	return XModelSurfs.XPakKey;
}

uint64 FModernWarfare6SP::GetXyzOffset(const FMW6XSurface& Surface)
{
	return Surface.XyzOffset;
}

uint64 FModernWarfare6SP::GetTangentFrameOffset(const FMW6XSurface& Surface)
{
	return Surface.TangentFrameOffset;
}

uint64 FModernWarfare6SP::GetTexCoordOffset(const FMW6XSurface& Surface)
{
	return Surface.TexCoordOffset;
}

uint64 FModernWarfare6SP::GetColorOffset(const FMW6XSurface& Surface)
{
	return Surface.ColorOffset;
}

uint64 FModernWarfare6SP::GetIndexDataOffset(const FMW6XSurface& Surface)
{
	return Surface.IndexDataOffset;
}

uint64 FModernWarfare6SP::GetPackedIndicesOffset(const FMW6XSurface& Surface)
{
	return Surface.PackedIndicesOffset;
}
