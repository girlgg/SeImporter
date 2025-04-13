#pragma once
#include "EditorFramework/AssetImportData.h"
#include "CastAssetImportData.generated.h"

UCLASS(BlueprintType, config=EditorPerProjectUserSettings, HideCategories=Object, abstract, MinimalAPI)
class UCastAssetImportData : public UAssetImportData
{
	GENERATED_BODY()
public:
	UPROPERTY()
	bool bImportAsScene;
};
