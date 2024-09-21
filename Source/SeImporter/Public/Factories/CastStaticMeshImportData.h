#pragma once
#include "CastMeshImportData.h"
#include "CastStaticMeshImportData.generated.h"

UCLASS(BlueprintType, config=EditorPerProjectUserSettings, AutoExpandCategories=(Options), MinimalAPI)
class UCastStaticMeshImportData : public UCastMeshImportData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, config, Category = Mesh, meta = (ToolTip = "If enabled, combines all meshes into a single mesh", ImportType = "StaticMesh"))
	uint32 bCombineMeshes : 1;
	
	static UCastStaticMeshImportData* GetImportDataForStaticMesh(UStaticMesh* StaticMesh,
	                                                             UCastStaticMeshImportData* TemplateForCreation);
};
