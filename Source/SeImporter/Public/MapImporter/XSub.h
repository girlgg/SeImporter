#pragma once

class FLargeMemoryReader;

struct FXSubBlock
{
	uint8 CompressionType;
	uint32 CompressedSize;
	uint32 DecompressedSize;
	uint32 BlockOffset;
	uint32 DecompressedOffset;
	uint32 Unknown;
};

struct FXSubPackageCacheObject
{
	uint64 Offset;
	uint64 CompressedSize;
	uint64 UncompressedSize;
	FString RelativePath;

	friend FArchive& operator<<(FArchive& Ar, FXSubPackageCacheObject& Obj)
	{
		Ar << Obj.Offset;
		Ar << Obj.CompressedSize;
		Ar << Obj.UncompressedSize;
		Ar << Obj.RelativePath;
		return Ar;
	}
};

struct FXSubFileMeta
{
	FDateTime LastModified;
	FString FileHash;

	friend FArchive& operator<<(FArchive& Ar, FXSubFileMeta& Meta)
	{
		Ar << Meta.LastModified;
		Ar << Meta.FileHash;
		return Ar;
	}
};

class FXSub
{
public:
	struct FXSubAssetCache
	{
		TMap<uint64, FXSubPackageCacheObject> CacheObjects; // 全局资源索引
		TMap<FString, FXSubFileMeta> FileMetas; // 文件元数据（哈希、时间戳）

		friend FArchive& operator<<(FArchive& Ar, FXSubAssetCache& Cache)
		{
			Ar << Cache.CacheObjects;
			Ar << Cache.FileMetas;
			return Ar;
		}
	};

	void LoadFilesAsync(const FString& GamePath);
	void LoadFiles();
	void ReadXSub(FLargeMemoryReader& Reader, const FString& FilePath, int32 FileIndex,
	              TMap<uint64, FXSubPackageCacheObject>& LocalCache);
	TArray<uint8> ExtractXSubPackage(uint64 Key, uint32 Size);
	void RemoveInvalidEntries(const FString& RemovedFilePath);

	template <typename Func>
	auto AccessCache(Func&& Accessor);

	void SaveAssetCache();
	void LoadAssetCache();

	bool NeedsReload(const FString& FilePath);

	FString ComputeOptimizedHash(const FString& FilePath);

private:
	FRWLock CacheLock;

	FString SharedGamePath;

	TMap<uint64, FXSubPackageCacheObject> CacheObjects;
	TMap<FString, FXSubFileMeta> FileMetas;

	std::atomic<bool> IsLoading{false};
	FEvent* LoadCompletedEvent{nullptr};
};

template <typename Func>
auto FXSub::AccessCache(Func&& Accessor)
{
	// 如果正在加载，等待完成
	if (IsLoading.load())
	{
		LoadCompletedEvent->Wait();
		FPlatformProcess::ReturnSynchEventToPool(LoadCompletedEvent);
		LoadCompletedEvent = nullptr;
	}

	FRWScopeLock ReadLock(CacheLock, SLT_ReadOnly);
	return Accessor(CacheObjects);
}
