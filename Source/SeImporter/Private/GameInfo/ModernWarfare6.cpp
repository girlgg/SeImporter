#include "GameInfo/ModernWarfare6.h"

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
		FMW6MaterialTextureDef TextureDef = Process->ReadMemory<FMW6MaterialTextureDef>( Material.TextureTable + i * sizeof(FMW6MaterialTextureDef));
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
	return Surface.OverrideScale != -1 ? FVector3f::Zero() : FVector3f(Surface.OffsetsX, Surface.OffsetsY, Surface.OffsetsZ);
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
