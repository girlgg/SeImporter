#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UserMeshOptions.generated.h"

UENUM(BlueprintType)
enum class EMeshType : uint8
{
	/** Represents a Static Mesh option */
	StaticMesh UMETA(DisplayName = "Static Mesh"),
	/** Represents a Skeletal Mesh option */
	// TODO 导入骨骼网格体
	// SkeletalMesh UMETA(DisplayName = "Skeletal Mesh")
};

UCLASS(config = Engine, defaultconfig, transient)
class SEIMPORTER_API UUserMeshOptions : public UObject
{
public:
	GENERATED_BODY()

	// This property defines the type of mesh to import.
	// It is only editable if 'bAutomaticallyDecideMeshType' is set to false.
	UPROPERTY(EditAnywhere, Category = "Mesh Settings", meta = (DisplayName = "Mesh Type"))
	EMeshType MeshType{EMeshType::StaticMesh};

	UPROPERTY(EditAnywhere, Category = "Material Settings", meta = (DisplayName = "Material Image Format"))
	FString MaterialImageFormat{"png"};

	// Override the current master material by providing a custom master material.
	// Set your created master material here if you want to use a custom material for this mesh.
	UPROPERTY(EditAnywhere, Category = "Material Settings",
		meta = (EditCondition = "bImportMaterials == true", DisplayName = "Override Master Material",
			Tooltip =
			"Override the current master material by providing a custom master material. If you want to use a custom material for this mesh, set your created master material here."
		))
	TSoftObjectPtr<UMaterial> OverrideMasterMaterial{nullptr};

	/*UPROPERTY(EditAnywhere, Category = "Skeletal Mesh Settings",
		meta = (EditCondition = "MeshType == EMeshType::SkeletalMesh"))
	TSoftObjectPtr<USkeleton> OverrideSkeleton{nullptr};*/
	/*UPROPERTY(EditAnywhere, Category = "Skeletal Mesh Settings",
		meta = (EditCondition = "MeshType == EMeshType::SkeletalMesh", Tooltip =
			"Overrides the skeleton Roll axis to face the actual mesh front axis. Older games use -90, Newer games use 90. Try both and see which one fixes it"
		))
	float OverrideSkeletonRootRoll = 90.0f;*/

	bool bInitialized;
};
