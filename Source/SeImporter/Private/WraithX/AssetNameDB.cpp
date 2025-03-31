#include "WraithX/AssetNameDB.h"
#include "Misc/Crc.h"

FAssetNameDB& FAssetNameDB::Get()
{
	static FAssetNameDB Instance;
	return Instance;
}

void FAssetNameDB::Initialize()
{
	FScopeLock Lock(&DatabaseCriticalSection);

	if (!Database.IsValid())
	{
		DBPath = GetDatabasePath();
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		FString DBDir = FPaths::GetPath(DBPath);
		if (!PlatformFile.DirectoryExists(*DBDir))
		{
			PlatformFile.CreateDirectory(*DBDir);
		}

		if (OpenInsert())
		{
			// 键值对表
			Database.Execute(TEXT("CREATE TABLE IF NOT EXISTS AssetNameCache (Hash INTEGER PRIMARY KEY, Value TEXT);"));
			// 文件监控表
			Database.Execute(TEXT(R"(CREATE TABLE IF NOT EXISTS TrackedFiles (
	FileId INTEGER PRIMARY KEY AUTOINCREMENT,
	RelativePath TEXT UNIQUE,  -- 相对插件目录的路径
	FullPathHash INTEGER,      -- 完整路径的哈希值（用于快速比对）
	ContentHash INTEGER,       -- 文件内容的哈希值
	LastModifiedTime INTEGER   -- 最后修改时间（Unix时间戳）
);)"));
			Database.Execute(TEXT("PRAGMA journal_mode=WAL;"));
			Database.Execute(TEXT("PRAGMA synchronous=NORMAL;"));
			bIsRunning = true;

			// 启动异步工作线程
			AsyncWorker = MakeUnique<FAssetNameDBWorker>(*this);

			Database.Execute(
				TEXT("CREATE INDEX IF NOT EXISTS IDX_TrackedFiles_RelativePath ON TrackedFiles(RelativePath);"));

			// 处理文件追踪
			ProcessTrackedFiles();
		}
	}
}

void FAssetNameDB::Shutdown()
{
	bIsRunning = false;

	if (AsyncWorker.IsValid())
	{
		AsyncWorker->Stop();
		AsyncWorker.Reset();
	}

	FScopeLock Lock(&DatabaseCriticalSection);
	if (Database.IsValid())
	{
		Database.Close();
	}
}

bool FAssetNameDB::OpenInsert()
{
	if (!Database.IsValid())
	{
		return Database.Open(*DBPath, ESQLiteDatabaseOpenMode::ReadWriteCreate);
	}
	return true;
}

bool FAssetNameDB::InsertOrUpdate(uint64 Hash, const FString& Value)
{
	return ExecuteStatement(
		TEXT("INSERT OR REPLACE INTO AssetNameCache (Hash, Value) VALUES (?, ?);"),
		[Hash, &Value](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(Hash));
			Stmt.SetBindingValueByIndex(2, Value);
			return Stmt.Execute();
		}
	);
}

bool FAssetNameDB::BatchInsertOrUpdate(const TMap<uint64, FString>& Items)
{
	FScopeLock Lock(&DatabaseCriticalSection);
	if (!Database.IsValid() || Items.Num() == 0) return false;

	// 使用事务提升性能
	if (!Database.Execute(TEXT("BEGIN TRANSACTION;")))
		return false;

	FSQLitePreparedStatement Statement(
		Database, TEXT("INSERT OR REPLACE INTO AssetNameCache (Hash, Value) VALUES (?, ?);"));
	if (!Statement.IsValid())
	{
		Database.Execute(TEXT("ROLLBACK TRANSACTION;"));
		return false;
	}

	bool bSuccess = true;
	for (const auto& Item : Items)
	{
		Statement.Reset();
		Statement.SetBindingValueByIndex(1, static_cast<int64>(Item.Key));
		Statement.SetBindingValueByIndex(2, Item.Value);
		if (!Statement.Execute())
		{
			bSuccess = false;
			break;
		}
	}

	if (bSuccess)
		Database.Execute(TEXT("COMMIT TRANSACTION;"));
	else
		Database.Execute(TEXT("ROLLBACK TRANSACTION;"));

	return bSuccess;
}

bool FAssetNameDB::QueryValue(uint64 Hash, FString& OutValue)
{
	// 对于同步查询，仍然使用主数据库连接
	return ExecuteStatement(
		TEXT("SELECT Value FROM AssetNameCache WHERE Hash = ?;"),
		[Hash, &OutValue](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(Hash));
			if (Stmt.Step() == ESQLitePreparedStatementStepResult::Row)
			{
				Stmt.GetColumnValueByIndex(0, OutValue);
				return true;
			}
			return false;
		}
	);
}

bool FAssetNameDB::DeleteByHash(uint64 Hash)
{
	return ExecuteStatement(
		TEXT("DELETE FROM AssetNameCache WHERE Hash = ?;"),
		[Hash](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(Hash));
			const bool bSuccess = Stmt.Execute();
			return bSuccess;
		}
	);
}

void FAssetNameDB::InsertOrUpdateAsync(uint64 Hash, const FString& Value)
{
	if (AsyncWorker.IsValid())
	{
		AsyncWorker->EnqueueInsertOrUpdate(Hash, Value);
	}
}

void FAssetNameDB::BatchInsertOrUpdateAsync(const TMap<uint64, FString>& Items)
{
	if (AsyncWorker.IsValid())
	{
		AsyncWorker->EnqueueBatchInsertOrUpdate(Items);
	}
}

void FAssetNameDB::QueryValueAsync(uint64 Hash, TFunction<void(const FString&)> Callback)
{
	// 对于异步查询，使用并行读取
	if (AsyncWorker.IsValid())
	{
		AsyncWorker->EnqueueQuery(Hash, Callback);
	}
}

void FAssetNameDB::DeleteByHashAsync(uint64 Hash)
{
	if (AsyncWorker.IsValid())
	{
		AsyncWorker->EnqueueDelete(Hash);
	}
}

FAssetNameDB::FAssetNameDB(): bIsRunning(false)
{
}

FAssetNameDB::~FAssetNameDB()
{
	Shutdown();
}

FString FAssetNameDB::GetDatabasePath() const
{
	return FPaths::ProjectSavedDir() / TEXT("IWToUE") / "AssetName.db";
}

bool FAssetNameDB::ExecuteStatement(const FString& Query, TFunction<bool(FSQLitePreparedStatement&)> BindAndExecute)
{
	FScopeLock Lock(&DatabaseCriticalSection);
	if (!Database.IsValid()) return false;

	FSQLitePreparedStatement Statement(Database, *Query);
	if (Statement.IsValid())
	{
		bool bResult = BindAndExecute(Statement);
		Statement.Reset();
		return bResult;
	}
	return false;
}

void FAssetNameDB::ProcessTrackedFiles()
{
	TArray<FString> Files = FindFilesToTrack();
	for (const FString& FilePath : Files)
	{
		FString RelativePath = GetRelativePath(FilePath);
		uint64 FullPathHash = FCrc::StrCrc32(*FilePath);
		uint64 ContentHash = ComputeFileContentHash(FilePath);
		int64 LastModifiedTime = GetFileLastModifiedTime(FilePath);

		uint64 ExistingContentHash = 0;
		int64 ExistingLastModifiedTime = 0;

		bool bNeedsUpdate = CheckIfFileNeedsUpdate(RelativePath, ContentHash, LastModifiedTime, ExistingContentHash,
		                                           ExistingLastModifiedTime);
		if (bNeedsUpdate)
		{
			TMap<uint64, FString> Items;
			ParseFile(FilePath, Items);
			if (Items.Num() > 0)
			{
				BatchInsertOrUpdate(Items);
			}
			UpdateTrackedFile(RelativePath, FullPathHash, ContentHash, LastModifiedTime);
		}
	}
}

TArray<FString> FAssetNameDB::FindFilesToTrack() const
{
	TArray<FString> Files;
	FString TrackedDir = FPaths::ProjectPluginsDir() / TEXT("SeImporter");
	IFileManager::Get().FindFilesRecursive(Files, *TrackedDir, TEXT("*.wni"), true, false);
	return Files;
}

FString FAssetNameDB::GetRelativePath(const FString& FullPath) const
{
	FString PluginDir = FPaths::ProjectPluginsDir();
	FString RelativePath = FullPath;
	FPaths::MakePathRelativeTo(RelativePath, *PluginDir);
	return RelativePath;
}

uint64 FAssetNameDB::ComputeFileContentHash(const FString& FilePath) const
{
	TArray<uint8> Data;
	if (FFileHelper::LoadFileToArray(Data, *FilePath))
	{
		return FCrc::MemCrc32(Data.GetData(), Data.Num());
	}
	return 0;
}

int64 FAssetNameDB::GetFileLastModifiedTime(const FString& FilePath) const
{
	FDateTime TimeStamp = IFileManager::Get().GetTimeStamp(*FilePath);
	return TimeStamp.ToUnixTimestamp();
}

void FAssetNameDB::ParseFile(const FString& FilePath, TMap<uint64, FString>& Items)
{
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *FilePath)) return;

	FMemoryReader Ar(FileData, true);
	uint32 Magic;
	Ar << Magic;
	if (Magic != 0x20494E57) // 'WNI '
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid file format: %s"), *FilePath);
		return;
	}

	// 版本号
	Ar.Seek(Ar.Tell() + 2);

	// 条目数
	uint32 EntryCount;
	Ar << EntryCount;

	// 压缩大小
	uint32 PackedSize, UnpackedSize;
	Ar << PackedSize;
	Ar << UnpackedSize;

	// 校验剩余数据大小
	const int64 RemainingSize = Ar.TotalSize() - Ar.Tell();
	if (RemainingSize < PackedSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("Corrupted file: %s"), *FilePath);
		return;
	}

	// 读取压缩数据
	TArray<uint8> CompressedData;
	CompressedData.SetNumUninitialized(PackedSize);
	Ar.Serialize(CompressedData.GetData(), PackedSize);

	// 准备解压缓冲区
	TArray<uint8> DecompressedData;
	const int32 PaddedSize = (UnpackedSize + 3) & ~3;
	DecompressedData.SetNumUninitialized(PaddedSize);

	// LZ4解压
	bool bDecompressSuccess = FCompression::UncompressMemory(
		NAME_LZ4,
		DecompressedData.GetData(),
		UnpackedSize,
		CompressedData.GetData(),
		CompressedData.Num(),
		COMPRESS_NoFlags
	);

	if (!bDecompressSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("Decompression failed: %s"), *FilePath);
		return;
	}

	const uint8* StreamPtr = DecompressedData.GetData();
	const int64 StreamLength = DecompressedData.Num();
	int64 CurrentPos = 0;

	// 验证解压后数据足够存放EntryCount
	if (StreamLength < static_cast<int64>(EntryCount * sizeof(uint64_t)))
	{
		UE_LOG(LogTemp, Error, TEXT("Corrupted decompressed data"));
		return;
	}
	auto CheckBounds = [](int64 Current, int64 Needed, int64 Total)
	{
		if (Current + Needed > Total)
		{
			UE_LOG(LogTemp, Error, TEXT("Data overflow: need %lld bytes at offset %lld"), Needed, Current);
			return false;
		}
		return true;
	};
	// 解析条目
	for (uint32 i = 0; i < EntryCount; ++i)
	{
		// 1. 读取Key（小端序处理）
		uint64 Key;
		if (!CheckBounds(CurrentPos, sizeof(uint64), StreamLength)) break;

		FMemory::Memcpy(&Key, StreamPtr + CurrentPos, sizeof(uint64));
		// Key = FGenericPlatformMisc::NetToHost64(Key); // 正确的字节序转换
		Key &= 0x0FFFFFFFFFFFFFFF; // 应用掩码
		CurrentPos += sizeof(uint64);

		// 2. 读取C风格字符串
		if (!CheckBounds(CurrentPos, 1, StreamLength)) break;

		FString Value;
		const ANSICHAR* StrStart = reinterpret_cast<const ANSICHAR*>(StreamPtr + CurrentPos);
		int32 MaxLen = StreamLength - CurrentPos;
		int32 StrLen = 0;

		// 手动查找null终止符
		while (StrLen < MaxLen && StrStart[StrLen] != '\0')
		{
			StrLen++;
		}

		if (StrLen == MaxLen)
		{
			UE_LOG(LogTemp, Warning, TEXT("Unterminated string at entry %d"), i);
			break;
		}

		Value = FString(StrLen, StrStart); // UTF-8转换
		CurrentPos += StrLen + 1; // 跳过null终止符

		Items.Add(Key, Value);
	}
}

bool FAssetNameDB::UpdateTrackedFile(const FString& RelativePath, uint64 FullPathHash, uint64 ContentHash,
                                     int64 LastModifiedTime)
{
	return ExecuteStatement(
		TEXT(
			"INSERT OR REPLACE INTO TrackedFiles (RelativePath, FullPathHash, ContentHash, LastModifiedTime) VALUES (?, ?, ?, ?);"),
		[&](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, RelativePath);
			Stmt.SetBindingValueByIndex(2, static_cast<int64>(FullPathHash));
			Stmt.SetBindingValueByIndex(3, static_cast<int64>(ContentHash));
			Stmt.SetBindingValueByIndex(4, LastModifiedTime);
			return Stmt.Execute();
		}
	);
}

bool FAssetNameDB::CheckIfFileNeedsUpdate(const FString& RelativePath, uint64 NewContentHash, int64 NewLastModifiedTime,
                                          uint64& ExistingContentHash, int64& ExistingLastModifiedTime)
{
	bool bExists = ExecuteStatement(
		TEXT("SELECT ContentHash, LastModifiedTime FROM TrackedFiles WHERE RelativePath = ?;"),
		[&](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, RelativePath);
			if (Stmt.Step() == ESQLitePreparedStatementStepResult::Row)
			{
				Stmt.GetColumnValueByIndex(0, ExistingContentHash);
				Stmt.GetColumnValueByIndex(1, ExistingLastModifiedTime);
				return true;
			}
			return false;
		}
	);

	return !bExists || (NewContentHash != ExistingContentHash) || (NewLastModifiedTime != ExistingLastModifiedTime);
}

FOnTaskCompletedDelegate& FAssetNameDB::GetTaskCompletedHandle()
{
	return AsyncWorker->OnTaskCompleted;
}

FAssetNameDBWorker::FAssetNameDBWorker(FAssetNameDB& InParent)
	: Parent(InParent), Thread(nullptr), bShouldRun(true)
{
	Thread = FRunnableThread::Create(this, TEXT("KeyValueDatabaseWorker"));
}

FAssetNameDBWorker::~FAssetNameDBWorker()
{
	Stop();
	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
	}
}

uint32 FAssetNameDBWorker::Run()
{
	while (bShouldRun)
	{
		TSharedPtr<FDatabaseTask> Task;
		{
			FScopeLock Lock(&QueueCriticalSection);
			if (!TaskQueue.IsEmpty())
			{
				TaskQueue.Dequeue(Task);
			}
		}

		if (Task.IsValid())
		{
			switch (Task->Type)
			{
			case FDatabaseTask::EType::Insert:
				Parent.InsertOrUpdate(Task->Hash, Task->Value);
				break;
			case FDatabaseTask::EType::Query:
				{
					FString Result;
					if (Parent.QueryValue(Task->Hash, Result) && Task->Callback)
					{
						AsyncTask(ENamedThreads::GameThread, [Callback = Task->Callback, Result]()
						{
							Callback(Result);
						});
					}
				}
				break;
			case FDatabaseTask::EType::Delete:
				Parent.DeleteByHash(Task->Hash);
				break;
			case FDatabaseTask::EType::BatchInsert:
				Parent.BatchInsertOrUpdate(Task->BatchItems);
				break;
			}
			if (TaskQueue.IsEmpty())
			{
				OnTaskCompleted.Broadcast();
				OnTaskCompleted.Clear();
			}
		}
		else
		{
			FPlatformProcess::Sleep(0.1f); // 避免忙等待
		}
	}
	return 0;
}

void FAssetNameDBWorker::Stop()
{
	bShouldRun = false;
}

void FAssetNameDBWorker::EnqueueInsertOrUpdate(uint64 Hash, const FString& Value)
{
	FScopeLock Lock(&QueueCriticalSection);
	auto Task = MakeShared<FDatabaseTask>();
	Task->Type = FDatabaseTask::EType::Insert;
	Task->Hash = Hash;
	Task->Value = Value;
	TaskQueue.Enqueue(Task);
}

void FAssetNameDBWorker::EnqueueBatchInsertOrUpdate(const TMap<uint64, FString>& Items)
{
	FScopeLock Lock(&QueueCriticalSection);
	auto Task = MakeShared<FDatabaseTask>();
	Task->Type = FDatabaseTask::EType::BatchInsert;
	Task->BatchItems = Items; // 假设TMap支持拷贝，数据量大时需优化
	TaskQueue.Enqueue(Task);
}

void FAssetNameDBWorker::EnqueueQuery(uint64 Hash, TFunction<void(const FString&)> Callback)
{
	FScopeLock Lock(&QueueCriticalSection);
	auto Task = MakeShared<FDatabaseTask>();
	Task->Type = FDatabaseTask::EType::Query;
	Task->Hash = Hash;
	Task->Callback = Callback;
	TaskQueue.Enqueue(Task);
}

void FAssetNameDBWorker::EnqueueDelete(uint64 Hash)
{
	FScopeLock Lock(&QueueCriticalSection);
	auto Task = MakeShared<FDatabaseTask>();
	Task->Type = FDatabaseTask::EType::Delete;
	Task->Hash = Hash;
	TaskQueue.Enqueue(Task);
}
