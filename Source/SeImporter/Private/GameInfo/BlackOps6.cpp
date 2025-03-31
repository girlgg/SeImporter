#include "GameInfo/BlackOps6.h"

FBlackOps6::FBlackOps6(FCordycepProcess* InProcess)
	: TBaseGame(InProcess)
{
	GFXMAP_POOL_IDX = 43;
}

TArray<FCastTextureInfo> FBlackOps6::PopulateMaterial(const FBO6Material& Material)
{
	TArray<FBO6GfxImage>Images;

	for (int i = 0; i < Material.ImageCount; i++)
	{
		const uint32 ImagePtr = Process->ReadMemory<uint32>(Material.ImageTable + i * 8);
		FBO6GfxImage Image = Process->ReadMemory<FBO6GfxImage>(ImagePtr);
		Images.Add(Image);
	}

	TArray<FCastTextureInfo> Textures;

	for (int i = 0; i < Material.TextureCount; i++)
	{
		FBO6MaterialTextureDef TextureDef = Process->ReadMemory<FBO6MaterialTextureDef>( Material.TextureTable + i * sizeof(FBO6MaterialTextureDef));
		FBO6GfxImage Image = Images[TextureDef.ImageIdx];

		int UVMapIndex = 0;

		uint64 Hash = Image.Hash & 0x0FFFFFFFFFFFFFFF;

		FString ImageName = FString::Printf(TEXT("ximage_%llX"), Hash);

		FString TextureSemantic = FString::Printf(TEXT("unk_semantic_0x%x"), TextureDef.Index);
		
		FCastTextureInfo TextureInfo;
		TextureInfo.TextureName = ImageName;
	}

	return Textures;
}

FVector3f FBlackOps6::GetSurfaceOffset(const FBO6XSurface& Surface)
{
	return Surface.OverrideScale != -1 ? FVector3f::Zero() : Surface.SurfBounds.MidPoint;
}

float FBlackOps6::GetSurfaceScale(const FBO6XSurface& Surface)
{
	return Surface.OverrideScale != -1 ? Surface.OverrideScale : FMath::Max3(Surface.SurfBounds.HalfSize.X, Surface.SurfBounds.HalfSize.Y, Surface.SurfBounds.HalfSize.Z);
}

uint64 FBlackOps6::GetXPakKey(const FBO6XModelSurfs& XModelSurfs, const FBO6XSurfaceShared& Shared)
{
	return Shared.XPakKey;
}

uint64 FBlackOps6::GetXyzOffset(const FBO6XSurface& Surface)
{
	return 0;
}

uint64 FBlackOps6::GetTangentFrameOffset(const FBO6XSurface& Surface)
{
	return 0;
}

uint64 FBlackOps6::GetTexCoordOffset(const FBO6XSurface& Surface)
{
	return 0;
}

uint64 FBlackOps6::GetColorOffset(const FBO6XSurface& Surface)
{
	return 0;
}

uint64 FBlackOps6::GetIndexDataOffset(const FBO6XSurface& Surface)
{
	return 0;
}

uint64 FBlackOps6::GetPackedIndicesOffset(const FBO6XSurface& Surface)
{
	return 0;
}
