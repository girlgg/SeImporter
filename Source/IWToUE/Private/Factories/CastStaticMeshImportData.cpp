#include "Factories/CastStaticMeshImportData.h"

UCastStaticMeshImportData* UCastStaticMeshImportData::GetImportDataForStaticMesh(UStaticMesh* StaticMesh,
	UCastStaticMeshImportData* TemplateForCreation)
{
	check(StaticMesh);

	UCastStaticMeshImportData* ImportData = Cast<UCastStaticMeshImportData>(StaticMesh->AssetImportData);
	if (!ImportData)
	{
		ImportData = NewObject<UCastStaticMeshImportData>(StaticMesh, NAME_None, RF_NoFlags, TemplateForCreation);

		if (StaticMesh->AssetImportData != NULL)
		{
			ImportData->SourceData = StaticMesh->AssetImportData->SourceData;
		}

		StaticMesh->AssetImportData = ImportData;
	}

	return ImportData;
}
