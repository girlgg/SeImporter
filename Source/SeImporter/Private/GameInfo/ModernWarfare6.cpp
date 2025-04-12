#include "GameInfo/ModernWarfare6.h"

#include "Structures/MW6SPGameStructures.h"
#include "Utils/CoDAssetHelper.h"
#include "Utils/CoDBonesHelper.h"
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

bool FModernWarfare6::ReadXModel(FWraithXModel& OutModel, TSharedPtr<FGameProcess> ProcessInstance,
                                 TSharedPtr<FCoDModel> ModelAddr)
{
	FMW6XModel ModelData = ProcessInstance->ReadMemory<FMW6XModel>(ModelAddr->AssetPointer);
	FMW6BoneInfo BoneInfo = ProcessInstance->ReadMemory<FMW6BoneInfo>(ModelData.BoneInfoPtr);

	OutModel.ModelName = ModelAddr->AssetName;

	if (BoneInfo.BoneParentsPtr)
	{
		OutModel.BoneCount = BoneInfo.NumBones /*+ BoneInfo.CosmeticBoneCount*/;
		OutModel.RootBoneCount = BoneInfo.NumRootBones;
	}

	OutModel.BoneRotationData = EBoneDataTypes::DivideBySize;
	OutModel.IsModelStreamed = true;
	OutModel.BoneIDsPtr = BoneInfo.BoneIDsPtr;
	OutModel.BoneIndexSize = sizeof(uint32);
	OutModel.BoneParentsPtr = BoneInfo.BoneParentsPtr;
	OutModel.BoneParentSize = sizeof(uint16);
	OutModel.RotationsPtr = BoneInfo.RotationsPtr;
	OutModel.TranslationsPtr = BoneInfo.TranslationsPtr;
	OutModel.BaseTransformPtr = BoneInfo.BaseMatriciesPtr;

	for (uint32 LodIdx = 0; LodIdx < ModelData.NumLods; ++LodIdx)
	{
		FMW6XModelLod ModelLod = ProcessInstance->ReadMemory<FMW6XModelLod>(ModelData.LodInfo);

		FWraithXModelLod& LodRef = OutModel.ModelLods.AddDefaulted_GetRef();

		LodRef.LodDistance = ModelLod.LodDistance;
		LodRef.LODStreamInfoPtr = ModelLod.MeshPtr;
		uint64 XSurfacePtr = ModelLod.SurfsPtr;

		LodRef.Submeshes.Reserve(ModelLod.NumSurfs);
		for (uint32 SurfaceIdx = 0; SurfaceIdx < ModelLod.NumSurfs; ++SurfaceIdx)
		{
			FWraithXModelSubmesh& SubMeshRef = LodRef.Submeshes.AddDefaulted_GetRef();

			FMW6XSurface Surface = ProcessInstance->ReadMemory<FMW6XSurface>(
				XSurfacePtr + SurfaceIdx * sizeof(FMW6XSurface));

			SubMeshRef.VertexCount = Surface.VertCount;
			SubMeshRef.FaceCount = Surface.TriCount;
			SubMeshRef.PackedIndexTableCount = Surface.PackedIndicesTableCount;
			SubMeshRef.VertexOffset = Surface.XyzOffset;
			SubMeshRef.VertexUVsOffset = Surface.TexCoordOffset;
			SubMeshRef.VertexTangentOffset = Surface.TangentFrameOffset;
			SubMeshRef.FacesOffset = Surface.IndexDataOffset;
			SubMeshRef.PackedIndexTableOffset = Surface.PackedIndiciesTableOffset;
			SubMeshRef.PackedIndexBufferOffset = Surface.PackedIndicesOffset;
			SubMeshRef.VertexColorOffset = Surface.VertexColorOffset;
			SubMeshRef.WeightsOffset = Surface.WeightsOffset;

			if (Surface.OverrideScale != -1)
			{
				SubMeshRef.Scale = Surface.OverrideScale;
				SubMeshRef.XOffset = 0;
				SubMeshRef.YOffset = 0;
				SubMeshRef.ZOffset = 0;
			}
			else
			{
				SubMeshRef.Scale = FMath::Max3(Surface.Min, Surface.Max, Surface.Scale);
				SubMeshRef.XOffset = Surface.OffsetsX;
				SubMeshRef.YOffset = Surface.OffsetsY;
				SubMeshRef.ZOffset = Surface.OffsetsZ;
			}
			for (int32 WeightIdx = 0; WeightIdx < 8; ++WeightIdx)
			{
				SubMeshRef.WeightCounts[WeightIdx] = Surface.WeightCounts[WeightIdx];
			}

			uint64 MaterialHandle = ProcessInstance->ReadMemory<uint64>(
				ModelData.MaterialHandles + SurfaceIdx * sizeof(uint64));

			FWraithXMaterial& Material = LodRef.Materials.AddDefaulted_GetRef();
			ReadXMaterial(Material, ProcessInstance, MaterialHandle);
		}
	}
	return true;
}

bool FModernWarfare6::ReadXMaterial(FWraithXMaterial& OutMaterial, TSharedPtr<FGameProcess> ProcessInstance,
                                    uint64 MaterialHandle)
{
	if (ProcessInstance->GetCurrentGameFlag() == CoDAssets::ESupportedGameFlags::SP)
	{
		FMW6SPMaterial MaterialData = ProcessInstance->ReadMemory<FMW6SPMaterial>(MaterialHandle);
	}
	else
	{
		FMW6Material MaterialData = ProcessInstance->ReadMemory<FMW6Material>(MaterialHandle);

		FCoDAssetDatabase::Get().
			AssetName_QueryValueRetName(MaterialData.Hash, OutMaterial.MaterialName,TEXT("ximage"));

		TArray<FMW6GfxImage> Images;
		Images.Reserve(MaterialData.ImageCount);
		for (uint32 i = 0; i < MaterialData.ImageCount; i++)
		{
			const uint64 ImagePtr = ProcessInstance->ReadMemory<uint64>(MaterialData.ImageTable + i * sizeof(uint64));
			FMW6GfxImage Image = ProcessInstance->ReadMemory<FMW6GfxImage>(ImagePtr);
			Images.Add(Image);
		}

		OutMaterial.Images.Empty();

		for (int i = 0; i < MaterialData.TextureCount; i++)
		{
			FMW6MaterialTextureDef TextureDef = ProcessInstance->ReadMemory<FMW6MaterialTextureDef>(
				MaterialData.TextureTable + i * sizeof(FMW6MaterialTextureDef));
			FMW6GfxImage Image = Images[TextureDef.ImageIdx];

			FString ImageName;

			FCoDAssetDatabase::Get().AssetName_QueryValueRetName(Image.Hash, ImageName,TEXT("ximage"));

			// 一些占位符之类的，没有用
			if (!ImageName.IsEmpty() && ImageName[0] != '$')
			{
				FWraithXImage& ImageOut = OutMaterial.Images.AddDefaulted_GetRef();
				ImageOut.ImageName = ImageName;
				ImageOut.SemanticHash = TextureDef.Index;
			}
		}
	}

	return true;
}

void FModernWarfare6::LoadXModel(TSharedPtr<FGameProcess> ProcessInstance, FWraithXModel& BaseModel,
                                 FWraithXModelLod& ModelLod, FCastModelInfo& ResultModel)
{
	FMW6XModelSurfs MeshInfo = ProcessInstance->ReadMemory<FMW6XModelSurfs>(ModelLod.LODStreamInfoPtr);
	FMW6XSurfaceShared BufferInfo = ProcessInstance->ReadMemory<FMW6XSurfaceShared>(MeshInfo.Shared);

	TArray<uint8> MeshDataBuffer;
	if (BufferInfo.Data)
	{
		ProcessInstance->ReadArray(BufferInfo.Data, MeshDataBuffer, BufferInfo.DataSize);
	}
	else
	{
		MeshDataBuffer = ProcessInstance->GetDecrypt()->ExtractXSubPackage(MeshInfo.XPakKey, BufferInfo.DataSize);
	}

	CoDBonesHelper::ReadXModelBones(ProcessInstance, BaseModel, ModelLod, ResultModel);

	if (MeshDataBuffer.IsEmpty())
	{
		return;
	}

	for (FWraithXModelSubmesh& Submesh : ModelLod.Submeshes)
	{
		FCastMeshInfo& Mesh = ResultModel.Meshes.AddDefaulted_GetRef();
		Mesh.MaterialIndex = Submesh.MaterialIndex;

		FBufferReader VertexPosReader(MeshDataBuffer.GetData() + Submesh.VertexOffset,
		                              Submesh.VertexOffset * sizeof(uint64), false);
		FBufferReader VertexTangentReader(MeshDataBuffer.GetData() + Submesh.VertexTangentOffset,
		                                  Submesh.VertexOffset * sizeof(uint32), false);
		FBufferReader VertexUVReader(MeshDataBuffer.GetData() + Submesh.VertexUVsOffset,
		                             Submesh.VertexOffset * sizeof(uint32), false);
		FBufferReader FaceIndiciesReader(MeshDataBuffer.GetData() + Submesh.FacesOffset,
		                                 Submesh.FacesOffset * sizeof(uint16) * 3, false);

		// 顶点权重
		Mesh.VertexWeights.SetNum(Submesh.VertexCount);
		int32 WeightDataLength = 0;
		for (int32 WeightIdx = 0; WeightIdx < 8; ++WeightIdx)
		{
			WeightDataLength += (WeightIdx + 1) * 4 * Submesh.WeightCounts[WeightIdx];
		}
		// TODO 多骨骼支持
		if (ResultModel.Skeletons.Num() > 0)
		{
			FBufferReader VertexWeightReader(MeshDataBuffer.GetData() + Submesh.WeightsOffset, WeightDataLength, false);
			uint32 WeightDataIndex = 0;
			for (int32 i = 0; i < 8; ++i)
			{
				for (int32 WeightIdx = 0; WeightIdx < i + 1; ++WeightIdx)
				{
					for (uint32 LocalWeightDataIndex = WeightDataIndex;
					     LocalWeightDataIndex < WeightDataIndex + Submesh.WeightCounts[i]; ++LocalWeightDataIndex)
					{
						Mesh.VertexWeights[LocalWeightDataIndex].WeightCount = WeightIdx + 1;
						uint16 BoneValue;
						VertexWeightReader << BoneValue;
						Mesh.VertexWeights[LocalWeightDataIndex].BoneValues[WeightIdx] = BoneValue;

						if (WeightIdx > 0)
						{
							uint16 WeightValue;
							VertexWeightReader << WeightValue;
							Mesh.VertexWeights[LocalWeightDataIndex].WeightValues[WeightIdx] = WeightValue / 65536.f;
							Mesh.VertexWeights[LocalWeightDataIndex].WeightValues[0] -= Mesh.VertexWeights[
									LocalWeightDataIndex].
								WeightValues[WeightIdx];
						}
						else
						{
							VertexWeightReader.Seek(VertexWeightReader.Tell() + 2);
						}
					}
				}
				WeightDataIndex += Submesh.WeightCounts[i];
			}
		}

		Mesh.VertexPositions.Reserve(Submesh.VertexCount);
		Mesh.VertexTangents.Reserve(Submesh.VertexCount);
		Mesh.VertexNormals.Reserve(Submesh.VertexCount);
		Mesh.VertexUV.Reserve(Submesh.VertexCount);
		Mesh.VertexColor.Reserve(Submesh.VertexCount);
		// UV
		for (uint32 VertexIdx = 0; VertexIdx < Submesh.VertexCount; ++VertexIdx)
		{
			FVector3f& VertexPos = Mesh.VertexPositions.AddDefaulted_GetRef();
			uint64 PackedPos;
			VertexPosReader << PackedPos;
			VertexPos = FVector3f{
				((PackedPos >> 00 & 0x1FFFFF) * (1.0f / 0x1FFFFF * 2.0f) - 1.0f) * Submesh.Scale + Submesh.XOffset,
				((PackedPos >> 21 & 0x1FFFFF) * (1.0f / 0x1FFFFF * 2.0f) - 1.0f) * Submesh.Scale + Submesh.YOffset,
				((PackedPos >> 42 & 0x1FFFFF) * (1.0f / 0x1FFFFF * 2.0f) - 1.0f) * Submesh.Scale + Submesh.ZOffset
			};

			uint32 PackedTangentFrame;
			VertexTangentReader << PackedTangentFrame;
			FVector3f Normal, Tangent;
			UnpackCoDQTangent(PackedTangentFrame, Tangent, Normal);
			Mesh.VertexNormals.Add(Normal);
			Mesh.VertexTangents.Add(Tangent);

			uint16 HalfUvU, HalfUvV;
			VertexUVReader << HalfUvU << HalfUvV;
			FFloat16 HalfFloatU, HalfFloatV;
			HalfFloatU.Encoded = HalfUvU;
			HalfFloatV.Encoded = HalfUvV;
			Mesh.VertexUV.Emplace(HalfUvU, HalfFloatU);
		}
		// TODO Second UV
		// 顶点色
		if (Submesh.VertexColorOffset != 0xFFFFFFFF)
		{
			FBufferReader VertexColorReader(MeshDataBuffer.GetData() + Submesh.VertexColorOffset,
			                                Submesh.FacesOffset * sizeof(uint32), false);
			for (uint32 VertexIdx = 0; VertexIdx < Submesh.VertexCount; VertexIdx++)
			{
				uint32 VertexColor;
				VertexColorReader << VertexColor;
				Mesh.VertexColor.Add(VertexColor);
			}
		}
		// 面
		uint64 TableOffsetPtr = reinterpret_cast<uint64>(MeshDataBuffer.GetData()) + Submesh.PackedIndexTableOffset;
		uint64 PackedIndices = reinterpret_cast<uint64>(MeshDataBuffer.GetData()) + Submesh.PackedIndexBufferOffset;
		uint64 IndicesPtr = reinterpret_cast<uint64>(MeshDataBuffer.GetData()) + Submesh.FacesOffset;

		for (uint32 TriIdx = 0; TriIdx < Submesh.FaceCount; TriIdx++)
		{
			TArray<uint16> Faces;
			FCoDMeshHelper::UnpackFaceIndices(ProcessInstance, Faces, TableOffsetPtr, Submesh.PackedIndexTableCount,
			                                  PackedIndices, IndicesPtr, TriIdx, true);

			Mesh.Faces.Add(Faces[2]);
			Mesh.Faces.Add(Faces[1]);
			Mesh.Faces.Add(Faces[0]);
		}
	}
}
