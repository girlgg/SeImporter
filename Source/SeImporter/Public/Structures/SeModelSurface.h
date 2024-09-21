#pragma once

class FLargeMemoryReader;

struct FSeModelWeight
{
	/** 顶点索引*/
	uint32 VertexIndex;
	/** 骨骼索引*/
	uint32 WeightID;
	/** 权重信息*/
	float WeightValue;
};

struct FSeModelVertexColor
{
	uint8 R, G, B, A;

	FColor ToFColor() const
	{
		return FColor(this->R, this->G, this->B, this->A);
	}

	FVector4f ToVector() const
	{
		return FVector4f(this->R / 255.0f, this->G / 255.0f, this->B / 255.0f, this->A / 255.0f);
	}

	friend FArchive& operator<<(FArchive& Ar, FSeModelVertexColor& Color)
	{
		Ar << Color.R;
		Ar << Color.G;
		Ar << Color.B;
		Ar << Color.A;
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
	                         const int GlobalSurfaceVertCounter, bool bUseUVs = true, bool bUseNormals = true,
	                         bool bUseColors = true, bool bUseWeights = true);

	FString SurfaceName;
	TArray<FSeModelVertex> Vertexes;
	TArray<FGfxFace> Faces;
	TArray<int32> Materials;
	uint8 UVCount{0};

	int32 SurfaceVertexCounter{0};
	uint32 BoneCountBuffer{0};
	uint8 Flags{0};
	uint8 MaterialReferenceCount{0};
	uint32 FaceCount{0};
	uint32 VertexCount{0};
	uint8 MaxSkinInfluence{0};

	TArray<FGfxFace> ParseFaces(FLargeMemoryReader& Reader) const;
};
