#pragma once

#include "CoreMinimal.h"
#include "CoDCDNDownloader.h"

class IWebClient;

struct FCoDCDNDownloaderV2Entry
{
	// The stream hash
	uint64_t Hash;
	// The size of the compressed buffer.
	uint64_t Size;
	// The flags assigned to the entry.
	uint64_t Flags;
};

class FCoDCDNDownloaderV2 : public FCoDCDNDownloader
{
public:
	virtual ~FCoDCDNDownloaderV2() override = default;

	virtual bool Initialize(const FString& GameDirectory) override;
	virtual int32 ExtractCDNObject(TArray<uint8>& Buffer, uint64 CacheID, int32 BufferSize) override;

	bool InitializeFileSystem(const FString& GameDirectory);
	bool LoadCDNXPak(const FString& FileName);
	void FindXpakFilesRecursively(const FString& Directory, TArray<FString>& OutFiles);

private:
	TMap<uint64, FCoDCDNDownloaderV2Entry> Entries;
	const FString CoDV2CDNURL = "http://cod-assets.cdn.blizzard.com/pc/iw9_2";

	TSharedPtr<IWebClient> Client;
	// TUniquePtr<ICasCDNFileSystem> FileSystem;
};
