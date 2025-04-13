#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

struct FCoDCDNCacheEntry
{
	// The hash of the object stored.
	uint64 Hash;
	// The offset within the data file.
	uint64 Offset;
	// The size of the object 
	uint64 Size;
};

class FCoDCDNCache
{
public:
	bool Load(const FString& Name);
	bool Save();
	bool Add(const uint64 Hash, const TArray<uint8>& Buffer);
	bool Extract(uint64 Hash, int32 ExpectedSize, TArray<uint8>& OutBuffer);

private:
	FRWLock CacheLock;
	FString InfoFilePath;
	FString DataFilePath;
	TMap<uint64, FCoDCDNCacheEntry> Entries;
	bool bIsLoaded = false;
};
