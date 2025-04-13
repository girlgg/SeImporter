#include "CDN/CoDCDNCache.h"

bool FCoDCDNCache::Load(const FString& Name)
{
	FString CdnDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("IWToUE"), TEXT("cdn_cache"));

	InfoFilePath = FPaths::Combine(CdnDir, Name + TEXT(".cdn_info"));
	DataFilePath = FPaths::Combine(CdnDir, Name + TEXT(".cdn_data"));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectory(*CdnDir);

	bIsLoaded = true;

	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *InfoFilePath))
		return false;
    
	FMemoryReader Reader(FileData, true);
    
	uint32 Signature;
	Reader << Signature;
	if (Signature != 0x494E4443)
		return false;
    
	uint32 Version;
	Reader << Version;
	if (Version != 1)
		return false;
    
	uint32 NumEntries;
	Reader << NumEntries;
    
	for (uint32 i = 0; i < NumEntries; i++)
	{
		FCoDCDNCacheEntry Entry;
		Reader << Entry.Hash;
		Reader << Entry.Offset;
		Reader << Entry.Size;
		Entries.Add(Entry.Hash, Entry);
	}
    
	return true;
}

bool FCoDCDNCache::Save()
{
	if (!bIsLoaded)
		return false;
    
	TArray<uint8> Data;
	FMemoryWriter Writer(Data);
    
	uint32 Signature = 0x494E4443;
	uint32 Version = 1;
	Writer << Signature;
	Writer << Version;
    
	uint32 NumEntries = Entries.Num();
	Writer << NumEntries;
    
	for (auto& EntryPair : Entries)
	{
		const FCoDCDNCacheEntry& Entry = EntryPair.Value;
		Writer.Serialize((void*)&Entry.Hash, sizeof(Entry.Hash));
		Writer.Serialize((void*)&Entry.Offset, sizeof(Entry.Offset));
		Writer.Serialize((void*)&Entry.Size, sizeof(Entry.Size));
	}
    
	return FFileHelper::SaveArrayToFile(Data, *InfoFilePath);
}

bool FCoDCDNCache::Add(const uint64 Hash, const TArray<uint8>& Buffer)
{
	if (!bIsLoaded)
		return false;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
	TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenWrite(*DataFilePath, true, false));
	if (!FileHandle)
		return false;

	FCoDCDNCacheEntry Entry;
	Entry.Hash = Hash;
	Entry.Offset = FileHandle->Tell();
	Entry.Size = Buffer.Num();

	Entries.Add(Entry.Hash, Entry);

	if (!Save())
	{
		return false;
	}

	if (!FileHandle->Write(Buffer.GetData(), Buffer.Num()))
	{
		return false;
	}

	return true;
}

bool FCoDCDNCache::Extract(uint64 Hash, int32 ExpectedSize, TArray<uint8>& OutBuffer)
{
	if (!bIsLoaded)
		return false;

	const FCoDCDNCacheEntry* Entry = Entries.Find(Hash);
	if (!Entry)
		return false;

	if (ExpectedSize > 0 && Entry->Size != ExpectedSize)
		return false;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*DataFilePath));
	if (!FileHandle)
		return false;

	if (!FileHandle->Seek(Entry->Offset))
		return false;

	OutBuffer.SetNumUninitialized(Entry->Size);
    
	if (!FileHandle->Read(OutBuffer.GetData(), Entry->Size))
	{
		OutBuffer.Empty();
		return false;
	}
	return true;
}
