#include "Structures/SeModelSurface.h"
#include "Serialization/LargeMemoryReader.h"
#include "Utils/BinaryReader.h"

FSeModelSurface::FSeModelSurface(FLargeMemoryReader& Reader, uint32_t BufferBoneCount, uint16_t SurfaceCount,
                                 const int GlobalSurfaceVertCounter, bool bUseUVs, bool bUseNormals,
                                 bool bUseColors, bool bUseWeights)
{
	SurfaceVertexCounter = GlobalSurfaceVertCounter;
	BoneCountBuffer = BufferBoneCount;

	Reader << Flags;
	Reader << MaterialReferenceCount;
	Reader << MaxSkinInfluence;

	if (!bUseUVs)
	{
		MaterialReferenceCount = 0;
	}
	if (!bUseWeights)
	{
		MaxSkinInfluence = 0;
	}

	Reader << VertexCount;
	Reader << FaceCount;

	// 读取顶点
	Vertexes.SetNum(VertexCount);
	for (auto& Vertex : Vertexes)
	{
		Reader << Vertex.Position;
	}
	for (auto& Vertex : Vertexes)
	{
		Reader << Vertex.UV;
	}
	for (auto& Vertex : Vertexes)
	{
		Reader << Vertex.Normal;
	}
	for (auto& Vertex : Vertexes)
	{
		Reader << Vertex.Color;
	}
	for (uint32 i = 0; i < VertexCount; ++i)
	{
		Vertexes[i].Weights.SetNum(MaxSkinInfluence);
		for (FSeModelWeight& Weight : Vertexes[i].Weights)
		{
			if (BoneCountBuffer <= 0xFF)
			{
				uint8_t WeightID;
				Reader << WeightID;
				Weight.WeightID = WeightID;
			}
			else if (BoneCountBuffer <= 0xFFFF)
			{
				uint16_t WeightID;
				Reader << WeightID;
				Weight.WeightID = WeightID;
			}
			else
			{
				uint32_t WeightID;
				Reader << WeightID;
				Weight.WeightID = WeightID;
			}
			Reader << Weight.WeightValue;
			Weight.VertexIndex = i + SurfaceVertexCounter;
		}
	}

	// 读取面
	Faces = ParseFaces(Reader);
	// 读取材质
	Materials = FBinaryReader::ReadList<int32>(Reader, MaterialReferenceCount);

	SurfaceName = FString::Format(TEXT("surf_{0}"), {SurfaceCount});
}

TArray<FGfxFace> FSeModelSurface::ParseFaces(FLargeMemoryReader& Reader) const
{
	TArray<FGfxFace> RetFaces;
	RetFaces.SetNum(FaceCount);
	for (FGfxFace& Face : RetFaces)
	{
		if (VertexCount <= 0xFF)
		{
			uint8 Index1, Index2, Index3;
			Reader << Index3;
			Reader << Index2;
			Reader << Index1;
			Face.Index[0] = Index1 + SurfaceVertexCounter;
			Face.Index[1] = Index2 + SurfaceVertexCounter;
			Face.Index[2] = Index3 + SurfaceVertexCounter;
		}
		else if (VertexCount <= 0xFFFF)
		{
			uint16 Index1, Index2, Index3;
			Reader << Index3;
			Reader << Index2;
			Reader << Index1;
			Face.Index[0] = Index1 + SurfaceVertexCounter;
			Face.Index[1] = Index2 + SurfaceVertexCounter;
			Face.Index[2] = Index3 + SurfaceVertexCounter;
		}
		else
		{
			uint32 Index1, Index2, Index3;
			Reader << Index1;
			Reader << Index2;
			Reader << Index3;
			Face.Index[0] = Index1 + SurfaceVertexCounter;
			Face.Index[1] = Index2 + SurfaceVertexCounter;
			Face.Index[2] = Index3 + SurfaceVertexCounter;
		}
	}
	return RetFaces;
}
