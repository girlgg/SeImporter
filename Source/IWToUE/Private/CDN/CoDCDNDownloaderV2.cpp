#include "CDN/CoDCDNDownloaderV2.h"

#include "CDN/WebClientFactory.h"
#include "CDN/XSubCacheV2.h"
#include "CDN/XSubCacheV3.h"
#include "Serialization/LargeMemoryReader.h"

bool FCoDCDNDownloaderV2::Initialize(const FString& GameDirectory)
{
	if (bIsInitialized) return true;

	Client = FWebClientFactory::Create();

	TSharedPtr<FDownloadMemoryResult> Result = Client->DownloadData(CoDV2CDNURL);
	if (!Result.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to connect CDN server"));
		return false;
	}

	// 初始化文件系统
	if (!InitializeFileSystem(GameDirectory))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to initialize file system for directory: %s"), *GameDirectory);
		return false;
	}

	Cache.Load("CoDCDNV2");

	return bIsInitialized = true;
}

int32 FCoDCDNDownloaderV2::ExtractCDNObject(TArray<uint8>& Buffer, uint64 CacheID, int32 BufferSize)
{
	if (!Entries.Contains(CacheID))
	{
		return 0;
	}
	FCoDCDNDownloaderV2Entry Entry = Entries[CacheID];
	FRWScopeLock WriteLock(CDNLock, SLT_Write);

	TArray<uint8> CDNBuffer;
	Cache.Extract(CacheID, Entry.Size, CDNBuffer);
	if (!CDNBuffer.IsEmpty())
	{
		if (FXSubCacheV2::DecompressPackageObject(Entry.Hash, CDNBuffer, BufferSize, Buffer))
		{
			return Buffer.Num();
		}
	}

	if (HasFailed(CacheID))
	{
		return 0;
	}

	FString URL = FString::Printf(TEXT("%s/23/%02x/%016llx_%08llx_%s"), *CoDV2CDNURL, static_cast<uint8>(Entry.Hash),
	                              Entry.Hash, Entry.Size, Entry.Flags ? TEXT("1") : TEXT("0"));
	TSharedPtr<FDownloadMemoryResult> Result = Client->DownloadData(URL);
	if (Result->DataBuffer.IsEmpty() || Result->DataBuffer.Num() != Entry.Size)
	{
		AddFiled(CacheID);
		return 0;
	}
	Cache.Add(CacheID, Result->DataBuffer);

	if (FXSubCacheV2::DecompressPackageObject(Entry.Hash, Result->DataBuffer, BufferSize, Buffer))
	{
		return Buffer.Num();
	}
	return 0;
}

bool FCoDCDNDownloaderV2::LoadCDNXPak(const FString& FileName)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TUniquePtr<IMappedFileHandle> MappedHandle(PlatformFile.OpenMapped(*FileName));
	if (!MappedHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create mapped file handle: %s"), *FileName);
		return false;
	}
	TUniquePtr<IMappedFileRegion> MappedRegion(MappedHandle->MapRegion(0, MappedHandle->GetFileSize(), false));
	if (!MappedRegion)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to map file region: %s"), *FileName);
		return false;
	}
	FLargeMemoryReader Reader(MappedRegion->GetMappedPtr(), MappedRegion->GetMappedSize(),
	                          ELargeMemoryReaderFlags::Persistent);

	FXSubHeaderV2 Header;
	Reader << Header;

	if (Header.Magic != 0x4950414b && Header.HashOffset >= static_cast<uint64>(MappedRegion->GetMappedSize()))
	{
		return false;
	}
	Reader.Seek(Header.HashOffset);

	for (uint64 i = 0; i < Header.HashCount; ++i)
	{
		FXSubHashEntryV2 PackedEntry;
		Reader << PackedEntry;

		FCoDCDNDownloaderV2Entry Entry;
		Entry.Hash = PackedEntry.Key;
		Entry.Size = (PackedEntry.PackedInfo >> 1) & 0x3FFFFFFF;
		Entry.Flags = PackedEntry.PackedInfo & 1;

		Entries.Add(Entry.Hash, Entry);
	}

	return true;
}

void FCoDCDNDownloaderV2::FindXpakFilesRecursively(const FString& Directory, TArray<FString>& OutFiles)
{
	IFileManager& FileManager = IFileManager::Get();

	TArray<FString> FileNames;
	FileManager.FindFiles(FileNames, *(Directory / TEXT("*cdn.xpak")), true, false);

	for (const FString& FileName : FileNames)
	{
		OutFiles.Add(Directory / FileName);
	}

	TArray<FString> SubDirs;
	FileManager.FindFiles(SubDirs, *(Directory / TEXT("*")), false, true);

	for (const FString& Dir : SubDirs)
	{
		FindXpakFilesRecursively(Directory / Dir, OutFiles);
	}
}

bool FCoDCDNDownloaderV2::InitializeFileSystem(const FString& GameDirectory)
{
	FString BuildInfoPath = FPaths::Combine(GameDirectory, TEXT(".build.info"));
	if (FPaths::FileExists(BuildInfoPath))
	{
		// TODO CASC
	}
	else
	{
	}
	if (!GameDirectory.IsEmpty())
	{
		TArray<FString> XpakFiles;
		FindXpakFilesRecursively(GameDirectory, XpakFiles);
		for (const FString& FilePath : XpakFiles)
		{
			LoadCDNXPak(FilePath);
		}
	}
	return true;
}
