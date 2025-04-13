#pragma once

#include "CoreMinimal.h"
#include "SQLiteDatabase.h"
#include "UObject/Object.h"

struct FXSubPackageCacheObject;
class FCoDAssetDBWorker;

DECLARE_MULTICAST_DELEGATE(FOnTaskCompletedDelegate);

class FCoDAssetDatabase : public FNoncopyable
{
public:
	static FCoDAssetDatabase& Get();

	/*!
	 * @brief 初始化数据库
	 */
	void Initialize();
	void Shutdown();

	void XSub_Initial(uint64 GameID, const FString& GamePath);
	void XSub_AddGamePath(uint64 GameID, const FString& GamePath);

	// 同步操作
	bool AssetName_InsertOrUpdate(uint64 Hash, const FString& Value);
	bool AssetName_BatchInsertOrUpdate(const TMap<uint64, FString>& Items);
	bool AssetName_QueryValue(uint64 Hash, FString& OutValue);
	void AssetName_QueryValueRetName(uint64 Hash, FString& OutValue, const FString& NamePrefix = TEXT(""));
	bool AssetName_DeleteByHash(uint64 Hash);

	bool XSub_BatchInsertOrUpdate(const TMap<uint64, FXSubPackageCacheObject>& Items);
	// 根据Key查询文件，路径自动拼接为绝对路径
	bool XSub_QueryValue(uint64 Key, FXSubPackageCacheObject& OutValue);
	

	// 异步操作
	void InsertOrUpdateAsync(uint64 Hash, const FString& Value);
	void BatchInsertOrUpdateAsync(const TMap<uint64, FString>& Items);
	void QueryValueAsync(uint64 Hash, TFunction<void(const FString&)> Callback);
	void DeleteByHashAsync(uint64 Hash);

private:
	FCoDAssetDatabase();
	~FCoDAssetDatabase();

	void CreateTables();

	FString GetDatabasePath() const;
	bool ExecuteStatement(const FString& Query, TFunction<bool(FSQLitePreparedStatement&)> BindAndExecute);

	//=================================================================================================
	// 文件追踪
	//=================================================================================================
	void ProcessTrackedFiles();
	TArray<FString> FindWniFilesToTrack() const;
	TArray<FString> FindXSubFilesToTrack() const;
	FString GetRelativePath(const FString& FullPath) const;
	uint64 ComputeFileContentHash(const FString& FilePath, int64 ComputeSize = 1024 * 1024) const;
	int64 GetFileLastModifiedTime(const FString& FilePath) const;
	void ParseWniFile(const FString& FilePath, TMap<uint64, FString>& Items);
	void ParseXSubFile(const FString& FilePath, TMap<uint64, FXSubPackageCacheObject>& Items, int64 FileId);
	bool UpdateTrackedWniFile(int64 FileId, const FString& RelativePath, uint64 ContentHash, int64 LastModifiedTime);
	bool UpdateTrackedXSubFile(int64 FileId, uint64 GameID, const FString& RelativePath, uint64 ContentHash, int64 LastModifiedTime);
	/*!
	 * @brief 检查文件是否需要更新，不存在则插入
	 */
	bool CheckIfFileNeedsUpdate(const uint64 GameHash, const FString& RelativePath, uint64 NewContentHash,
	                            int64 NewLastModifiedTime, uint64& ExistingContentHash,
	                            int64& ExistingLastModifiedTime, int64& FileId);

public:
	FOnTaskCompletedDelegate& GetTaskCompletedHandle();

private:
	FSQLiteDatabase Database;
	FCriticalSection DatabaseCriticalSection;
	TUniquePtr<FCoDAssetDBWorker> AsyncWorker;
	FThreadSafeBool bIsRunning;
	FString DBPath;

	uint64 CoDGameHash;
	FString CoDGamePath;

	bool bIsInitialized = false;

	friend class FCoDAssetDBWorker;
};

// 异步工作线程
class FCoDAssetDBWorker : public FRunnable
{
public:
	FCoDAssetDBWorker(FCoDAssetDatabase& InParent);
	virtual ~FCoDAssetDBWorker();

	virtual uint32 Run() override;
	virtual void Stop() override;

	void EnqueueInsertOrUpdate(uint64 Hash, const FString& Value);
	void EnqueueBatchInsertOrUpdate(const TMap<uint64, FString>& Items);
	void EnqueueQuery(uint64 Hash, TFunction<void(const FString&)> Callback);
	void EnqueueDelete(uint64 Hash);

	FOnTaskCompletedDelegate OnTaskCompleted;

private:
	struct FDatabaseTask
	{
		enum class EType { Insert, BatchInsert, Query, Delete };

		EType Type;
		uint64 Hash;
		FString Value;
		TMap<uint64, FString> BatchItems; // 新增批量数据
		TFunction<void(const FString&)> Callback;
	};

	FCoDAssetDatabase& Parent;
	FRunnableThread* Thread;
	FThreadSafeBool bShouldRun;
	FCriticalSection QueueCriticalSection;
	TQueue<TSharedPtr<FDatabaseTask>> TaskQueue;
};
