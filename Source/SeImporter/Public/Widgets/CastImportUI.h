#pragma once

#include "CastImportUI.generated.h"

UENUM(BlueprintType)
enum class ECastImportType : uint8
{
	Cast_StaticMesh UMETA(DisplayName="Static Mesh"),
	Cast_SkeletalMesh UMETA(DisplayName="Skeletal Mesh"),
	Cast_Animation UMETA(DisplayName="Animation"),

	Cast_Max,
};

UENUM(BlueprintType)
enum class EMaterialType : uint8
{
	CastMT_IW9 UMETA(DisplayName="IW9 Engine"),
	CastMT_IW8 UMETA(DisplayName="IW8 Engine")
};

UENUM(BlueprintType)
enum class ECastAnimImportType : uint8
{
	CastAIT_Auto UMETA(DisplayName="Automatically detect"),
	CastAIT_Absolutely UMETA(DisplayName="Force as absolute"),
	CastAIT_Relative UMETA(DisplayName="Force as relative")
};

UENUM(BlueprintType)
enum class ECastTextureImportType : uint8
{
	CastTIT_Default UMETA(DisplayName="Model/Material/Image"),
	CastTIT_GlobalMaterials UMETA(DisplayName="GlobalMaterials/Material/Image"),
	CastTIT_GlobalImages UMETA(DisplayName="GlobalImages/Image")
};

UCLASS(BlueprintType, config=EditorPerProjectUserSettings, AutoExpandCategories=(FTransform), HideCategories=Object,
	MinimalAPI)
class UCastImportUI : public UObject
{
	GENERATED_BODY()

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Mesh, meta=(ImportType="SkeletalMesh|SkeletalMesh"))
	TObjectPtr<class USkeleton> Skeleton{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh,
		meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="As Skeletal Mesh"))
	bool bImportAsSkeletal{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Mesh, meta=(ImportType="SkeletalMesh"))
	bool bImportMesh{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Mesh, meta=(ImportType="SkeletalMesh", EditCondition="bImportMesh"))
	bool bReverseFace{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, config, Category=Mesh,
		meta=(ImportType="SkeletalMesh"))
	bool bPhysicsAsset{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category=Mesh,
		meta=(ImportType="SkeletalMesh", EditCondition="bPhysicsAsset"))
	TObjectPtr<class UPhysicsAsset> PhysicsAsset{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category=Animation, meta=(ImportType="SkeletalMesh|Animation"))
	bool bImportAnimations{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category=Animation, meta=(ImportType="SkeletalMesh|Animation", EditCondition="bImportAnimations"))
	bool bImportAnimationNotify{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category=Animation, meta=(ImportType="SkeletalMesh|Animation", EditCondition="bImportAnimations"))
	bool bDeleteRootNodeAnim{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category=Animation, meta=(ImportType="SkeletalMesh|Animation", EditCondition="bImportAnimations"))
	bool bConvertRefPosition{true};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category=Animation, meta=(ImportType="SkeletalMesh|Animation", EditCondition="bImportAnimations"))
	bool bConvertRefAnim{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material, meta=(ImportType="GeoOnly"))
	bool bMaterials{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material, meta=(ImportType="GeoOnly", EditCondition="bMaterials"))
	EMaterialType MaterialType{EMaterialType::CastMT_IW9};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material,
		meta=(ImportType="GeoOnly", EditCondition="bMaterials"))
	ECastTextureImportType TexturePathType{ECastTextureImportType::CastTIT_Default};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material,
		meta=(ImportType="GeoOnly", EditCondition="TexturePathType!=ECastTextureImportType::CastTIT_Default"))
	FString GlobalMaterialPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material, meta=(ImportType="GeoOnly"))
	FString TextureFormat{"png"};

	void ResetToDefault();
};
