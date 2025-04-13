#pragma once

#include "CoreMinimal.h"
#include "CoDCDNCache.h"
#include "UObject/Object.h"

class FCoDCDNDownloader
{
public:
	virtual ~FCoDCDNDownloader() = default;

	virtual bool Initialize(const FString& GameDirectory) = 0;
	/*!
	 * @param Buffer 传出字节序列 
	 * @return 传出字节数
	 */
	virtual int32 ExtractCDNObject(TArray<uint8>& Buffer, uint64 CacheID, int32 BufferSize) = 0;
	virtual void AddFiled(const uint64 CacheID);
	virtual bool HasFailed(const uint64 CacheID);

protected:
	FCoDCDNCache Cache;
	TMap<uint64, FDateTime> FailedAttempts;
	FRWLock CDNLock;
	bool bIsCDNAvailable = false;
	bool bIsInitialized = false;
};
