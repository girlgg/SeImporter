#pragma once

class FAssetImportManager;
class IGameAssetHandler;
struct FCoDAsset;

class IAssetImporter
{
public:
	virtual ~IAssetImporter() = default;

	virtual bool Import(const FString& ImportPath, TSharedPtr<FCoDAsset> Asset, IGameAssetHandler* Handler,
	                    FAssetImportManager* Manager) = 0;
};
