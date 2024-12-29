#pragma once

#include "CastImportUI.generated.h"

UENUM(BlueprintType)
enum ECastImportType : int8
{
	Cast_StaticMesh UMETA(DisplayName="Static Mesh"),
	Cast_SkeletalMesh UMETA(DisplayName="Skeletal Mesh"),
	Cast_Animation UMETA(DisplayName="Animation"),

	Cast_Max,
};

UENUM(BlueprintType)
enum EMaterialType : int8
{
	CastMT_IW9 UMETA(DisplayName="IW9 Engine"),
	CastMT_IW8 UMETA(DisplayName="IW8 Engine")
};

UENUM(BlueprintType)
enum ECastAnimImportType : uint8
{
	CastAIT_Auto UMETA(DisplayName="Automatically detect"),
	CastAIT_Absolutely UMETA(DisplayName="Force as absolute"),
	CastAIT_Relative UMETA(DisplayName="Force as relative")
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

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category=Animation, meta=(ImportType="SkeletalMesh|Animation", EditCondition="bImportAnimations"))
	TEnumAsByte<ECastAnimImportType> AnimImportType{CastAIT_Auto};*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category=Animation, meta=(ImportType="SkeletalMesh|Animation", EditCondition="bImportAnimations"))
	bool bConvertRefPosition{true};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category=Animation, meta=(ImportType="SkeletalMesh|Animation", EditCondition="bImportAnimations"))
	bool bConvertRefAnim{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material, meta=(ImportType="GeoOnly"))
	bool bMaterials{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material, meta=(ImportType="GeoOnly", EditCondition="bMaterials"))
	TEnumAsByte<EMaterialType> MaterialType{CastMT_IW9};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material,
		meta=(ImportType="GeoOnly", EditCondition="bMaterials"))
	bool bUseGlobalMaterialsPath{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material,
		meta=(ImportType="GeoOnly", EditCondition="bUseGlobalMaterialsPath"))
	FString GlobalMaterialPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = Material, meta=(ImportType="GeoOnly"))
	FString TextureFormat{"png"};

	void ResetToDefault();
};
