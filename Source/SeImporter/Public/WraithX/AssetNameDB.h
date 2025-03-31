#pragma once

#include "CoreMinimal.h"
#include "SQLiteDatabase.h"
#include "UObject/Object.h"

class FAssetNameDBWorker;

DECLARE_MULTICAST_DELEGATE(FOnTaskCompletedDelegate);

class FAssetNameDB
{
public:
	static FAssetNameDB& Get();

	/*!
	 * @brief 初始化数据库，初始化完毕后自动关闭读权限
	 */
	void Initialize();
	void Shutdown();

	/*!
	 * @brief 开启插入
	 */
	bool OpenInsert();

	// 同步操作
	bool InsertOrUpdate(uint64 Hash, const FString& Value);
	bool BatchInsertOrUpdate(const TMap<uint64, FString>& Items);
	bool QueryValue(uint64 Hash, FString& OutValue);
	bool DeleteByHash(uint64 Hash);

	// 异步操作
	void InsertOrUpdateAsync(uint64 Hash, const FString& Value);
	void BatchInsertOrUpdateAsync(const TMap<uint64, FString>& Items);
	void QueryValueAsync(uint64 Hash, TFunction<void(const FString&)> Callback);
	void DeleteByHashAsync(uint64 Hash);

private:
	FAssetNameDB();
	~FAssetNameDB();

	FString GetDatabasePath() const;
	bool ExecuteStatement(const FString& Query, TFunction<bool(FSQLitePreparedStatement&)> BindAndExecute);

	//=================================================================================================
	// 文件追踪
	//=================================================================================================
	void ProcessTrackedFiles();
	TArray<FString> FindFilesToTrack() const;
	FString GetRelativePath(const FString& FullPath) const;
	uint64 ComputeFileContentHash(const FString& FilePath) const;
	int64 GetFileLastModifiedTime(const FString& FilePath) const;
	void ParseFile(const FString& FilePath, TMap<uint64, FString>& Items);
	bool UpdateTrackedFile(const FString& RelativePath, uint64 FullPathHash, uint64 ContentHash,
	                       int64 LastModifiedTime);
	bool CheckIfFileNeedsUpdate(const FString& RelativePath, uint64 NewContentHash, int64 NewLastModifiedTime,
	                            uint64& ExistingContentHash, int64& ExistingLastModifiedTime);

public:
	FOnTaskCompletedDelegate& GetTaskCompletedHandle();

private:
	FSQLiteDatabase Database;
	FCriticalSection DatabaseCriticalSection;
	TUniquePtr<FAssetNameDBWorker> AsyncWorker;
	FThreadSafeBool bIsRunning;
	FString DBPath;

	friend class FAssetNameDBWorker;
};

// 异步工作线程
class FAssetNameDBWorker : public FRunnable
{
public:
	FAssetNameDBWorker(FAssetNameDB& InParent);
	virtual ~FAssetNameDBWorker();

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

	FAssetNameDB& Parent;
	FRunnableThread* Thread;
	FThreadSafeBool bShouldRun;
	FCriticalSection QueueCriticalSection;
	TQueue<TSharedPtr<FDatabaseTask>> TaskQueue;
};
