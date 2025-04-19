#pragma once
#include "Interface/IAssetImporter.h"

class FMaterialImporter final: public IAssetImporter
{
public:
	virtual bool Import(const FString& ImportPath, TSharedPtr<FCoDAsset> Asset, IGameAssetHandler* Handler,
	                    FAssetImportManager* Manager) override;
};
