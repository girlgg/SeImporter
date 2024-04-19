#include "Structures/SeModelSurface.h"
#include "Serialization/LargeMemoryReader.h"

FSeModelSurface::FSeModelSurface(FLargeMemoryReader& Reader, uint32_t BufferBoneCount, uint16_t SurfaceCount,
                                 int GlobalSurfaceVertCounter)
{
	SurfaceVertexCounter = GlobalSurfaceVertCounter;
	BoneCountBuffer = BufferBoneCount;

	Reader << Empty;
	Reader << MaterialCount;
	Reader << MaxSkinBuffer;
	Reader << VertCount;
	Reader << FaceCount;

	Name = FString::Format(TEXT("surf_{0}"), {SurfaceCount});

	for (size_t i = 0; i < VertCount; i++)
	{
		FSeModelVertex Vertex;
		Reader << Vertex.Position;
		Vertexes.Add(Vertex);
	}

	for (size_t i = 0; i < VertCount; i++)
	{
		Reader << Vertexes[i].UV;
	}
	for (size_t i = 0; i < VertCount; i++)
	{
		Reader << Vertexes[i].Normal;
	}
	for (size_t i = 0; i < VertCount; i++)
	{
		Reader << Vertexes[i].Color;
	}
	for (size_t i = 0; i < VertCount; i++)
	{
		Vertexes[i].Weights = ParseWeight(Reader, i);
	}

	Faces = ParseFaces(Reader);
	Materials = FBinaryReader::ReadList<int32_t>(Reader, MaterialCount);
}

TArray<FGfxFace> FSeModelSurface::ParseFaces(FLargeMemoryReader& Reader) const
{
	TArray<FGfxFace> RetFaces;
	for (uint32_t i = 0; i < FaceCount; i++)
	{
		FGfxFace Face;
		if (VertCount <= 0xFF)
		{
			uint8_t Index1, Index2, Index3;
			Reader << Index3;
			Reader << Index2;
			Reader << Index1;
			Face.Index[0] = Index1 + SurfaceVertexCounter;
			Face.Index[1] = Index2 + SurfaceVertexCounter;
			Face.Index[2] = Index3 + SurfaceVertexCounter;
		}
		else if (VertCount <= 0xFFFF)
		{
			uint16_t Index1, Index2, Index3;
			Reader << Index3;
			Reader << Index2;
			Reader << Index1;
			Face.Index[0] = Index1 + SurfaceVertexCounter;
			Face.Index[1] = Index2 + SurfaceVertexCounter;
			Face.Index[2] = Index3 + SurfaceVertexCounter;
		}
		else
		{
			int Index1, Index2, Index3;
			Reader << Index1;
			Reader << Index2;
			Reader << Index3;
			Face.Index[0] = Index1 + SurfaceVertexCounter;
			Face.Index[1] = Index2 + SurfaceVertexCounter;
			Face.Index[2] = Index3 + SurfaceVertexCounter;
		}
		RetFaces.Add(Face);
	}
	return RetFaces;
}

TArray<FSeModelWeight> FSeModelSurface::ParseWeight(FLargeMemoryReader& Reader, const uint32_t CurrentVertIndex) const
{
	TArray<FSeModelWeight> C2Weights;
	for (uint32_t i = 0; i < MaxSkinBuffer; i++)
	{
		FSeModelWeight Weight;
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
		Weight.VertexIndex = CurrentVertIndex + SurfaceVertexCounter;
		C2Weights.Add(Weight);
	}
	return C2Weights;
}
