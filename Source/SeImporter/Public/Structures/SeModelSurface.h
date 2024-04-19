#pragma once

#include "Utils/BinaryReader.h"

class FLargeMemoryReader;

struct FSeModelWeight
{
	uint32_t VertexIndex;
	uint32_t WeightID;
	float WeightValue;
};

struct FSeModelVertexColor
{
	uint8_t r, g, b, a;

	FColor ToFColor() const
	{
		return FColor(this->r, this->g, this->b, this->a);
	}

	FVector4f ToVector() const
	{
		return FVector4f(this->r / 255.0f, this->g / 255.0f, this->b / 255.0f, this->a / 255.0f);
	}

	friend FArchive& operator<<(FArchive& Ar, FSeModelVertexColor& Color)
	{
		Ar << Color.r;
		Ar << Color.g;
		Ar << Color.b;
		Ar << Color.a;
		return Ar;
	}
};

struct FSeModelVertex
{
	FVector3f Position;
	FVector3f Normal;
	FVector2f UV;
	FSeModelVertexColor Color;
	TArray<FSeModelWeight> Weights;
};

struct FGfxFace
{
	uint32_t Index[3];
};

class SEIMPORTER_API FSeModelSurface
{
public:
	explicit FSeModelSurface(FLargeMemoryReader& Reader, uint32_t BufferBoneCount, uint16_t SurfaceCount,
	                        int GlobalSurfaceVertCounter);
	
	FString Name;
	TArray<FSeModelVertex> Vertexes;
	TArray<FGfxFace> Faces;
	TArray<int32_t> Materials;
	uint8_t UVCount{0};

	int SurfaceVertexCounter;
	uint32_t BoneCountBuffer;
	uint8_t Empty;
	uint8_t MaterialCount;
	uint32_t FaceCount;
	uint32_t VertCount;
	uint8_t MaxSkinBuffer;

	TArray<FGfxFace> ParseFaces(FLargeMemoryReader& Reader) const;
	TArray<FSeModelWeight> ParseWeight(FLargeMemoryReader& Reader, const uint32_t CurrentVertIndex) const;
};
