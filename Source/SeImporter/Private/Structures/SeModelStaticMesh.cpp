#include "Structures/SeModelStaticMesh.h"

#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "IMeshBuilderModule.h"
#include "MaterialDomain.h"
#include "MeshDescription.h"
#include "ObjectTools.h"
#include "StaticMeshAttributes.h"
#include "StaticToSkeletalMeshConverter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "Structures/SeModel.h"
#include "Structures/SeModelHeader.h"
#include "Structures/SeModelMaterialInstance.h"
#include "Structures/SeModelSurface.h"
#include "Structures/SeModelTexture.h"
#include "Widgets/UserMeshOptions.h"

UObject* SeModelStaticMesh::CreateMesh(UObject* ParentPackage, SeModel* InMesh,
                                       const TArray<FSeModelMaterial*>& CoDMaterials) const
{
	if (MeshOptions->MeshType == EMeshType::StaticMesh)
	{
		const FMeshDescription MeshDescription = CreateMeshDescription(InMesh);
		return CreateStaticMeshFromMeshDescription(ParentPackage, MeshDescription, InMesh,
		                                           CoDMaterials);
	}
	/*if (MeshOptions->MeshType == EMeshType::SkeletalMesh)
	{
		return CreateSkeletalMesh(ParentPackage, InMesh);
	}*/
	return nullptr;
}

FMeshDescription SeModelStaticMesh::CreateMeshDescription(SeModel* InMesh)
{
	FMeshDescription MeshDescription;
	FStaticMeshAttributes CombinedMeshAttributes{MeshDescription};
	CombinedMeshAttributes.Register();
	CombinedMeshAttributes.RegisterTriangleNormalAndTangentAttributes();

	const TVertexAttributesRef<FVector3f> TargetVertexPositions = CombinedMeshAttributes.GetVertexPositions();
	const TPolygonGroupAttributesRef<FName> PolygonGroupNames = CombinedMeshAttributes.
		GetPolygonGroupMaterialSlotNames();
	const TVertexInstanceAttributesRef<FVector3f> TargetVertexInstanceNormals =
		CombinedMeshAttributes.GetVertexInstanceNormals();
	const TVertexInstanceAttributesRef<FVector2f> TargetVertexInstanceUVs =
		CombinedMeshAttributes.GetVertexInstanceUVs();
	const TVertexInstanceAttributesRef<FVector4f> TargetVertexInstanceColors =
		CombinedMeshAttributes.GetVertexInstanceColors();

	const int32 VertexCount = InMesh->Header->HeaderBoneCount;
	TargetVertexInstanceUVs.SetNumChannels(InMesh->UVSetCount + 1);
	MeshDescription.ReserveNewVertices(InMesh->Header->HeaderBoneCount);
	MeshDescription.ReserveNewVertexInstances(InMesh->Header->HeaderBoneCount);
	MeshDescription.ReserveNewPolygons(InMesh->Header->HeaderMaterialCount);
	MeshDescription.ReserveNewEdges(InMesh->Header->HeaderMaterialCount * 2);
	TArray<FVertexID> VertexIndexToVertexID;
	VertexIndexToVertexID.Reserve(VertexCount);
	TArray<FVertexInstanceID> VertexIndexToVertexInstanceID;
	VertexIndexToVertexInstanceID.Reserve(VertexCount);
	int32 GlobalVertexIndex = 0;

	// 为每个 Surface 上的顶点创建顶点ID和顶点实例ID，并设置对应的位置、法线、颜色和UV坐标。
	for (const auto Surface : InMesh->Surfaces)
	{
		for (int i = 0; i < Surface->Vertexes.Num(); i++)
		{
			const FVertexID VertexID = MeshDescription.CreateVertex();
			TargetVertexPositions[VertexID] = FVector3f(Surface->Vertexes[i].Position.X,
			                                            -Surface->Vertexes[i].Position.Y,
			                                            Surface->Vertexes[i].Position.Z);
			VertexIndexToVertexID.Add(VertexID);

			const FVertexInstanceID VertexInstanceID = MeshDescription.CreateVertexInstance(VertexID);
			VertexIndexToVertexInstanceID.Add(VertexInstanceID);
			GlobalVertexIndex++;

			TargetVertexInstanceNormals[VertexInstanceID] = FVector3f(Surface->Vertexes[i].Normal.X,
			                                                          -Surface->Vertexes[i].Normal.Y,
			                                                          Surface->Vertexes[i].Normal.Z);
			TargetVertexInstanceColors[VertexInstanceID] = Surface->Vertexes[i].Color.ToVector();
			for (int u = 0; u < InMesh->UVSetCount; u++)
			{
				TargetVertexInstanceUVs.Set(VertexInstanceID, u,
				                            FVector2f(Surface->Vertexes[i].UV.X, Surface->Vertexes[i].UV.Y));
			}
		}
	}

	// 在网格模型中创建多边形，并将分配到不同的多边形组，同时为每个多边形组指定一个名称。
	for (const auto Surface : InMesh->Surfaces)
	{
		const FPolygonGroupID PolygonGroup = MeshDescription.CreatePolygonGroup();

		for (auto [FaceIndex] : Surface->Faces)
		{
			// 检查面的三个顶点索引是否有重复的，如果有，那么这个面就是一个退化面。
			if (FaceIndex[0] == FaceIndex[1] ||
				FaceIndex[1] == FaceIndex[2] ||
				FaceIndex[2] == FaceIndex[0])
			{
				continue;
			}

			TArray<FVertexInstanceID> VertexInstanceIDs;
			VertexInstanceIDs.SetNum(3);

			for (int i = 0; i < 3; i++)
			{
				const uint32_t VertexIndex = FaceIndex[i];
				VertexInstanceIDs[i] = VertexIndexToVertexInstanceID[VertexIndex];
			}

			MeshDescription.CreatePolygon(PolygonGroup, VertexInstanceIDs);
		}

		PolygonGroupNames[PolygonGroup] = FName(Surface->SurfaceName);
	}

	return MeshDescription;
}

UStaticMesh* SeModelStaticMesh::CreateStaticMeshFromMeshDescription(UObject* ParentPackage,
                                                                    const FMeshDescription& InMeshDescription,
                                                                    SeModel* InMesh,
                                                                    TArray<FSeModelMaterial*> CoDMaterials) const
{
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(ParentPackage, *InMesh->Header->MeshName,
	                                                 RF_Public | RF_Standalone);

	// Set default settings
	StaticMesh->InitResources();
	StaticMesh->SetLightingGuid();
	FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
	SrcModel.BuildSettings.bRecomputeNormals = false;
	SrcModel.BuildSettings.bRecomputeTangents = true;
	SrcModel.BuildSettings.bRemoveDegenerates = false;
	SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
	SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
	SrcModel.BuildSettings.bGenerateLightmapUVs = true;
	SrcModel.BuildSettings.SrcLightmapIndex = 0;
	SrcModel.BuildSettings.DstLightmapIndex = InMesh->UVSetCount; // We use last UV set for lightmap
	SrcModel.BuildSettings.bUseMikkTSpace = true;

	FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(0);

	if (!MeshDescription)
	{
		MeshDescription = StaticMesh->CreateMeshDescription(0);
	}
	*MeshDescription = InMeshDescription;
	StaticMesh->CommitMeshDescription(0);

	TArray<FStaticMaterial> StaticMaterials;
	// Create materials and mesh sections
	for (int i = 0; i < InMesh->Surfaces.Num(); i++)
	{
		const auto Surface = InMesh->Surfaces[i];

		// Static Material for Surface
		FStaticMaterial&& UEMat = FStaticMaterial(UMaterial::GetDefaultMaterial(MD_Surface));
		if (Surface->Materials.Num() > 0 && !CoDMaterials.IsEmpty())
		{
			// Create an array of Surface Materials
			TArray<FSeModelMaterial*> SurfMaterials;
			SurfMaterials.Reserve(Surface->Materials.Num());
			for (const uint16_t MaterialIndex : Surface->Materials)
			{
				SurfMaterials.Push(CoDMaterials[MaterialIndex]);
			}
			if (MeshOptions->OverrideMasterMaterial.IsValid())
			{
				UMaterialInterface* MaterialInstance = SeModelMaterialInstance::CreateMaterialInstance(
					SurfMaterials, ParentPackage, MeshOptions->OverrideMasterMaterial.LoadSynchronous());
				UEMat = FStaticMaterial(MaterialInstance);
			}
			else
			{
				UMaterialInterface* MaterialInstance = SeModelMaterialInstance::CreateMaterialInstance(
					SurfMaterials, ParentPackage, nullptr);
				UEMat = FStaticMaterial(MaterialInstance);
			}
		}
		UEMat.UVChannelData.bInitialized = true;
		UEMat.MaterialSlotName = FName(Surface->SurfaceName);
		UEMat.ImportedMaterialSlotName = FName(Surface->SurfaceName);
		StaticMaterials.Add(UEMat);
		StaticMesh->GetSectionInfoMap().Set(0, i, FMeshSectionInfo(i));
	}
	StaticMesh->SetStaticMaterials(StaticMaterials);
	StaticMesh->GetOriginalSectionInfoMap().CopyFrom(StaticMesh->GetSectionInfoMap());
	StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
	// Editor builds cache the mesh description so that it can be preserved during map reloads etc
	TArray<FText> BuildErrors;
	// Build mesh from source
	StaticMesh->Build(false);
	StaticMesh->EnforceLightmapRestrictions();
	StaticMesh->PostEditChange();
	StaticMesh->GetPackage()->FullyLoad();
	StaticMesh->MarkPackageDirty();

	// Notify asset registry of new asset
	FAssetRegistryModule::AssetCreated(StaticMesh);
	ThumbnailTools::GenerateThumbnailForObjectToSaveToDisk(StaticMesh);
	return StaticMesh;
}

USkeletalMesh* SeModelStaticMesh::CreateSkeletalMesh(UObject* ParentPackage, SeModel* InMesh) const
{
	USkeleton* Skeleton = nullptr;
	FReferenceSkeleton RefSkeleton;
	CreateSkeleton(InMesh, InMesh->Header->MeshName, ParentPackage, RefSkeleton, Skeleton);

	USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(ParentPackage, *InMesh->Header->MeshName,
	                                                       RF_Public | RF_Standalone);

	TArray<SkeletalMeshImportData::FRawBoneInfluence> Influences;
	for (const auto& Surface : InMesh->Surfaces)
	{
		for (const auto& Vertex : Surface->Vertexes)
		{
			for (const auto& [VertexIndex, WeightID, WeightValue] : Vertex.Weights)
			{
				if (WeightValue > 0)
				{
					SkeletalMeshImportData::FRawBoneInfluence Influence;
					Influence.BoneIndex = WeightID;
					Influence.VertexIndex = VertexIndex;
					Influence.Weight = WeightValue;
					Influences.Add(Influence);
				}
			}
		}
	}

	/*FStaticToSkeletalMeshConverter::InitializeSkeletalMeshFromStaticMesh(SkeletalMesh, Mesh, RefSkeleton, "");
	
	FSkeletalMeshImportData SkeletalMeshImportData;
	SkeletalMesh->LoadLODImportedData(0, SkeletalMeshImportData);
	TArray<SkeletalMeshImportData::FRawBoneInfluence> Influences;

	for (const auto& Surface : InMesh->Surfaces)
	{
		for (const auto& Vertex : Surface->Vertexes)
		{
			for (const auto& [VertexIndex, WeightID, WeightValue] : Vertex.Weights)
			{
				if (WeightValue > 0)
				{
					SkeletalMeshImportData::FRawBoneInfluence Influence;
					Influence.BoneIndex = WeightID;
					Influence.VertexIndex = VertexIndex;
					Influence.Weight = WeightValue;
					Influences.Add(Influence);
				}
			}
		}
	}
	SkeletalMeshImportData.Influences = Influences;
	SkeletalMesh->SaveLODImportedData(0, SkeletalMeshImportData);

	SkeletalMesh->CalculateInvRefMatrices();

	FSkeletalMeshBuildSettings BuildOptions;
	BuildOptions.bRemoveDegenerates = true;
	BuildOptions.bRecomputeTangents = true;
	BuildOptions.bUseMikkTSpace = true;
	SkeletalMesh->GetLODInfo(0)->BuildSettings = BuildOptions;
	SkeletalMesh->SetImportedBounds(FBoxSphereBounds(FBoxSphereBounds3f(FBox3f(SkeletalMeshImportData.Points))));

	auto& MeshBuilderModule = IMeshBuilderModule::GetForRunningPlatform();
	if (const FSkeletalMeshBuildParameters SkeletalMeshBuildParameters(
			SkeletalMesh,
			GetTargetPlatformManagerRef().
			GetRunningTargetPlatform(), 0, false);
		!MeshBuilderModule.BuildSkeletalMesh(SkeletalMeshBuildParameters))
	{
		SkeletalMesh->BeginDestroy();
		return nullptr;
	}

	SkeletalMesh->SetSkeleton(Skeleton);
	if (!MeshOptions->OverrideSkeleton.IsValid())
	{
		Skeleton->MergeAllBonesToBoneTree(SkeletalMesh);
		Skeleton->SetPreviewMesh(SkeletalMesh);
	}

	Skeleton->PostEditChange();
	FAssetRegistryModule::AssetCreated(Skeleton);
	Skeleton->GetPackage()->FullyLoad();
	Skeleton->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(SkeletalMesh);
	Skeleton->GetPackage()->FullyLoad();
	SkeletalMesh->MarkPackageDirty();*/

	return SkeletalMesh;
}

void SeModelStaticMesh::CreateSkeleton(SeModel* InMesh, FString ObjectName, UObject* ParentPackage,
                                       FReferenceSkeleton& OutRefSkeleton, USkeleton*& OutSkeleton) const
{
	FSkeletalMeshImportData SkeletonMeshImportData;

	for (auto [Name, Flags, ParentIndex, GlobalPosition, GlobalRotation, LocalPosition, LocalRotation, Scale] :
	     InMesh->Bones)
	{
		SkeletalMeshImportData::FBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = ParentIndex == -1 ? INDEX_NONE : ParentIndex;
		FVector3f PskBonePos = LocalPosition;
		auto PskBoneRotEu = LocalRotation;
		PskBoneRotEu.Yaw *= -1.0;
		PskBoneRotEu.Roll *= -1.0;
		if (Bone.Name == "j_mainroot")
		{
			// PskBoneRotEu.Roll = MeshOptions->OverrideSkeletonRootRoll;
		}
		FQuat4f PskBoneRot = PskBoneRotEu.Quaternion();
		FTransform3f PskTransform;

		PskTransform.SetLocation(FVector4f(PskBonePos.X, -PskBonePos.Y, PskBonePos.Z));
		PskTransform.SetRotation(PskBoneRot);
		SkeletalMeshImportData::FJointPos BonePos;
		BonePos.Transform = PskTransform;
		Bone.BonePos = BonePos;
		SkeletonMeshImportData.RefBonesBinary.Add(Bone);
	}
	FString NewName = "SK_" + ObjectName;
	auto SkeletonPackage = CreatePackage(*FPaths::Combine(FPaths::GetPath(ParentPackage->GetPathName()), NewName));
	OutSkeleton = /*MeshOptions->OverrideSkeleton.IsValid()
		              ? MeshOptions->OverrideSkeleton.LoadSynchronous()
		              :*/ NewObject<USkeleton>(SkeletonPackage, FName(*NewName), RF_Public | RF_Standalone);
	auto SkeletalDepth = 0;
	ProcessSkeleton(SkeletonMeshImportData, OutSkeleton, OutRefSkeleton, SkeletalDepth);
}

void SeModelStaticMesh::ProcessSkeleton(const FSkeletalMeshImportData& ImportData, const USkeleton* Skeleton,
                                        FReferenceSkeleton& OutRefSkeleton, int& OutSkeletalDepth)
{
	const auto RefBonesBinary = ImportData.RefBonesBinary;
	OutRefSkeleton.Empty();

	FReferenceSkeletonModifier RefSkeletonModifier(OutRefSkeleton, Skeleton);

	for (const auto [Name, Flags, NumChildren, ParentIndex, BonePos] : RefBonesBinary)
	{
		const FMeshBoneInfo BoneInfo(FName(*Name), Name, ParentIndex);
		RefSkeletonModifier.Add(BoneInfo, FTransform(BonePos.Transform));
	}

	OutSkeletalDepth = 0;

	TArray<int> SkeletalDepths;
	SkeletalDepths.Empty(ImportData.RefBonesBinary.Num());
	SkeletalDepths.AddZeroed(ImportData.RefBonesBinary.Num());
	for (auto b = 0; b < OutRefSkeleton.GetNum(); b++)
	{
		const auto Parent = OutRefSkeleton.GetParentIndex(b);
		auto Depth = 1.0f;

		SkeletalDepths[b] = 1.0f;
		if (Parent != INDEX_NONE)
		{
			Depth += SkeletalDepths[Parent];
		}
		if (OutSkeletalDepth < Depth)
		{
			OutSkeletalDepth = Depth;
		}
		SkeletalDepths[b] = Depth;
	}
}

UTexture2D* SeModelStaticMesh::ImportTexture(const FString& FilePath, const FString& ParentPath, bool bSRGB)
{
	UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
	ImportData->bReplaceExisting = true;

	const FString MaterialsPath = FPaths::Combine(*ParentPath, TEXT("Materials"));
	FString TexturePath = FPaths::Combine(*MaterialsPath,
	                                      TEXT("Textures"),
	                                      FPaths::GetCleanFilename(FPaths::GetPath(FilePath)));

	FString DestinationTexturePath = FPaths::Combine(TexturePath,
	                                                 FSeModelTexture::NoIllegalSigns(
		                                                 FPaths::GetCleanFilename(FilePath)));

	// Correct the check for an existing texture to use the proper path in the content directory
	FString AssetPath = DestinationTexturePath;
	if (UTexture2D* LoadedTexture = LoadObject<UTexture2D>(nullptr, *AssetPath))
	{
		return LoadedTexture;
	}

	// Set the proper destination path for the import data
	ImportData->DestinationPath = TexturePath;
	ImportData->Filenames.Add(FilePath);

	// Import the texture
	const FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
	if (ImportedAssets.Num() == 0)
	{
		return nullptr;
	}
	if (UTexture2D* ImportedTexture = Cast<UTexture2D>(ImportedAssets[0]))
	{
		if (!bSRGB)
		{
			ImportedTexture->CompressionSettings = TC_BC7;
			ImportedTexture->SRGB = false;
		}
		ImportedTexture->UpdateResource();
		ImportedTexture->GetPackage()->FullyLoad();
		ImportedTexture->MarkPackageDirty();
		return ImportedTexture;
	}
	return nullptr;
}

bool SeModelStaticMesh::ImportSeModelTexture(FSeModelTexture& SeModelTexture, const FString& ParentPath,
									 const FString& LineContent, const FString& BasePath, const FString& ImageFormat)
{
	TArray<FString> LineParts;
	LineContent.ParseIntoArray(LineParts, TEXT(","), false);
	FString TextureAddress = LineParts[0];
	FString TextureName = LineParts[1];
	SeModelTexture.TexturePath = FPaths::Combine(BasePath, TextureName + "." + ImageFormat);
	SeModelTexture.TextureName = TextureName;
	SeModelTexture.TextureType = TEXT("Normal");

	if (!FPaths::FileExists(SeModelTexture.TexturePath) || TextureName[0] == '$')
	{
		return false;
	}
	if (TextureAddress == "unk_semantic_0x0")
	{
		SeModelTexture.TextureSlot = "Albedo";
		SeModelTexture.TextureObject = ImportTexture(SeModelTexture.TexturePath, ParentPath, true);
		return true;
	}
	if (TextureAddress == "unk_semantic_0x4")
	{
		SeModelTexture.TextureSlot = "NOG";
		// 非彩色
		SeModelTexture.TextureObject = ImportTexture(SeModelTexture.TexturePath, ParentPath, false);
		return true;
	}
	if (TextureAddress == "unk_semantic_0x8")
	{
		SeModelTexture.TextureSlot = "Emission";
		SeModelTexture.TextureObject = ImportTexture(SeModelTexture.TexturePath, ParentPath, true);
		return true;
	}
	if (TextureAddress == "unk_semantic_0xC")
	{
		SeModelTexture.TextureSlot = "AlphaMask";
		SeModelTexture.TextureType = "Mask";
		// 非彩色
		SeModelTexture.TextureObject = ImportTexture(SeModelTexture.TexturePath, ParentPath, false);
		return true;
	}
	if (TextureAddress == "unk_semantic_0x22")
	{
		SeModelTexture.TextureSlot = "Transparency";
		SeModelTexture.TextureType = "Alpha";
		SeModelTexture.TextureObject = ImportTexture(SeModelTexture.TexturePath, ParentPath, true);
		return true;
	}
	if (TextureAddress == "unk_semantic_0x26")
	{
		// 没做，暂时不用
		SeModelTexture.TextureSlot = "Specular_Mask";
		// 非彩色
		SeModelTexture.TextureObject = ImportTexture(SeModelTexture.TexturePath, ParentPath, false);
		return true;
	}
	if (TextureAddress == "unk_semantic_0x32")
	{
		SeModelTexture.TextureSlot = "Mask";
		SeModelTexture.TextureObject = ImportTexture(SeModelTexture.TexturePath, ParentPath, true);
		return true;
	}

	return false;
}
