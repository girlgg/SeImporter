#pragma once

class FSeModelTexture;
class FSeModelMaterial;
class SeModel;
class UUserMeshOptions;

class SeModelStaticMesh
{
public:
	UUserMeshOptions* MeshOptions;

	UObject* CreateMesh(UObject* ParentPackage, SeModel* InMesh,
	                    const TArray<FSeModelMaterial*>& CoDMaterials) const;
	static FMeshDescription CreateMeshDescription(SeModel* InMesh);
	UStaticMesh* CreateStaticMeshFromMeshDescription(UObject* ParentPackage, const FMeshDescription& InMeshDescription,
	                                                 SeModel* InMesh, TArray<FSeModelMaterial*> CoDMaterials) const;
	USkeletalMesh* CreateSkeletalMesh(UObject* ParentPackage, SeModel* InMesh) const;
	void CreateSkeleton(SeModel* InMesh, FString ObjectName, UObject* ParentPackage,
	                    FReferenceSkeleton& OutRefSkeleton, USkeleton*& OutSkeleton) const;
	static void ProcessSkeleton(const FSkeletalMeshImportData& ImportData, const USkeleton* Skeleton,
	                            FReferenceSkeleton& OutRefSkeleton, int& OutSkeletalDepth);
	static UTexture2D* ImportTexture(const FString& FilePath, const FString& ParentPath, bool bSRGB);
	static bool ImportSeModelTexture(FSeModelTexture& SeModelTexture, const FString& ParentPath,
	                                 const FString& LineContent, const FString& BasePath, const FString& ImageFormat);
};
