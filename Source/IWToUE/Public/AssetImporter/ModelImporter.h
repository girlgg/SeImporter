#pragma once
#include "Interface/IAssetImporter.h"

class FModelImporter final: public IAssetImporter
{
public:
	virtual bool Import(const FString& ImportPath, TSharedPtr<FCoDAsset> Asset, IGameAssetHandler* Handler, FAssetImportManager* Manager) override;
};
