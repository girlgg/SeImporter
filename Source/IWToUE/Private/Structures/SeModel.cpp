#include "Structures/SeModel.h"
#include "Utils/BinaryReader.h"
#include "Serialization/LargeMemoryReader.h"
#include "Structures/SeModelHeader.h"
#include "Structures/SeModelSurface.h"

SeModel::SeModel(const FString& Filename, FLargeMemoryReader& Reader)
{
	// 初始文件头信息
	Header = new SeModelHeader(Filename, Reader);

	// 读取骨骼信息
	if (Header->DataPresentFlags & HeaderSpace::ESeModelDataPresenceFlags::SEMODEL_PRESENCE_BONE)
	{
		bool bUseGlobal =
			(Header->BonePresentFlags & HeaderSpace::ESeModelBonePresenceFlags::SEMODEL_PRESENCE_GLOBAL_MATRIX) > 0;
		bool bUseLocal =
			(Header->BonePresentFlags & HeaderSpace::ESeModelBonePresenceFlags::SEMODEL_PRESENCE_LOCAL_MATRIX) > 0;
		bool bUseScale =
			(Header->BonePresentFlags & HeaderSpace::ESeModelBonePresenceFlags::SEMODEL_PRESENCE_SCALES) > 0; 

		// 依次读取骨骼名
		Bones.SetNum(Header->HeaderBoneCount);
		for (auto& Bone : Bones)
		{
			FBinaryReader::ReadString(Reader, &Bone.Name);
		}
		// 依次读取骨骼信息
		for (auto& [Name, Flags, ParentIndex, GlobalPosition, GlobalRotation, LocalPosition, LocalRotation, Scale] :
		     Bones)
		{
			Reader << Flags;
			Reader << ParentIndex;
			if (bUseGlobal)
			{
				Reader << GlobalPosition;
				FQuat4f GlobalRotationQuat;
				Reader << GlobalRotationQuat;
				GlobalRotation = GlobalRotationQuat.Rotator();
			}
			if (bUseLocal)
			{
				Reader << LocalPosition;
				FQuat4f LocalRotationQuat;
				Reader << LocalRotationQuat;
				LocalRotation = LocalRotationQuat.Rotator();
			}
			if (bUseScale)
			{
				Reader << Scale;
			}
		}
	}

	// 读取网格信息
	if (Header->DataPresentFlags & HeaderSpace::ESeModelDataPresenceFlags::SEMODEL_PRESENCE_MESH)
	{
		bool bUseUVs =
			(Header->MeshPresentFlags & HeaderSpace::ESeModelMeshPresenceFlags::SEMODEL_PRESENCE_UVSET) > 0;
		bool bUseNormals =
			(Header->MeshPresentFlags & HeaderSpace::ESeModelMeshPresenceFlags::SEMODEL_PRESENCE_NORMALS) > 0;
		bool bUseColors =
			(Header->MeshPresentFlags & HeaderSpace::ESeModelMeshPresenceFlags::SEMODEL_PRESENCE_COLOR) > 0;
		bool bUseWeights =
			(Header->MeshPresentFlags & HeaderSpace::ESeModelMeshPresenceFlags::SEMODEL_PRESENCE_WEIGHTS) > 0;
		// 读取子网格
		Surfaces.SetNum(Header->HeaderMeshCount);
		for (uint32_t SurfaceIndex = 0; SurfaceIndex < Header->HeaderMeshCount; SurfaceIndex++)
		{
			Surfaces[SurfaceIndex] = new FSeModelSurface(Reader, Header->HeaderBoneCount, SurfaceIndex,
			                                               SurfaceVertCounter, bUseUVs, bUseNormals, bUseColors,
			                                               bUseWeights);
			SurfaceVertCounter += Surfaces[SurfaceIndex]->Vertexes.Num();
		}
	}

	// 读取材质信息
	if (Header->DataPresentFlags & HeaderSpace::ESeModelDataPresenceFlags::SEMODEL_PRESENCE_MATERIALS)
	{
		Materials.SetNum(Header->HeaderMaterialCount);
		for (FSeModelMeshMaterial& Material : Materials)
		{
			FBinaryReader::ReadString(Reader, &Material.MaterialName);
			// 不要SEModel文件存储的材质路径，后面会使用单独的读取材质的方法，这里的材质只读取不使用。
			// Don't store the material path from the SEModel. I use another method for store materials.
			for (size_t TextureTypeIndex = 0; TextureTypeIndex < 3; TextureTypeIndex++)
			{
				FString LocalTextureName;
				FBinaryReader::ReadString(Reader, &LocalTextureName);
			}
		}
	}
}
