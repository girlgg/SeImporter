#include "Structures/SeModel.h"
#include "Utils/BinaryReader.h"
#include "Serialization/LargeMemoryReader.h"
#include "Structures/SeModelHeader.h"
#include "Structures/SeModelSurface.h"

SeModel::SeModel(const FString& Filename, FLargeMemoryReader& Reader)
{
	// 初始文件头信息
	Header = new SeModelHeader(Filename, Reader);

	Surfaces.Reserve(Header->SurfaceCount);
	bIsXModel = true;

	// 依次读取骨骼名
	for (size_t BoneIndex = 0; BoneIndex < Header->BoneCountBuffer; BoneIndex++)
	{
		FSeModelMeshBone Bone;
		FBinaryReader::ReadString(Reader, &Bone.Name);
		Bones.Add(Bone);
	}
	// 依次读取骨骼信息
	for (auto& [Name, Non, ParentIndex, GlobalPosition, GlobalRotation, LocalPosition, LocalRotation] : Bones)
	{
		Reader << Non;
		Reader << ParentIndex;
		Reader << GlobalPosition;
		FQuat4f GlobalRotationQuat;
		Reader << GlobalRotationQuat;
		GlobalRotation = GlobalRotationQuat.Rotator();
		Reader << LocalPosition;
		FQuat4f LocalRotationQuat;
		Reader << LocalRotationQuat;
		LocalRotation = LocalRotationQuat.Rotator();
	}

	for (uint32_t SurfaceIndex = 0; SurfaceIndex < Header->SurfaceCount; SurfaceIndex++)
	{
		FSeModelSurface* Surface = new FSeModelSurface(Reader, Header->BoneCountBuffer, SurfaceIndex,
		                                               SurfaceVertCounter);
		Surfaces.Push(Surface);
		SurfaceVertCounter += Surface->Vertexes.Num();
	}

	for (size_t MaterialIndex = 0; MaterialIndex < Header->MaterialCountBuffer; MaterialIndex++)
	{
		FSeModelMeshMaterial Material;
		FBinaryReader::ReadString(Reader, &Material.MaterialName);

		// 不要SeModel内部存储的材质，后面会使用单独的读取材质的方法
		for (size_t TextureTypeIndex = 0; TextureTypeIndex < 3; TextureTypeIndex++)
		{
			FString LocalTextureName;
			FBinaryReader::ReadString(Reader, &LocalTextureName);
		}
		
		Materials.Add(Material);
	}
}
