#pragma once

#include "Interface/IAssetImporter.h"

class FImageImporter final : public IAssetImporter
{
	virtual bool Import(const FString& ImportPath, TSharedPtr<FCoDAsset> Asset, IGameAssetHandler* Handler,
	                    FAssetImportManager* Manager) override;
};
