#include "WraithX/CoDAssetDatabase.h"

#include "MapImporter/XSub.h"
#include "Misc/Crc.h"
#include "Serialization/LargeMemoryReader.h"
#include "Utils/BinaryReader.h"

FCoDAssetDatabase& FCoDAssetDatabase::Get()
{
	static FCoDAssetDatabase Instance;
	if (!Instance.bIsInitialized)
	{
		Instance.Initialize();
	}
	return Instance;
}

void FCoDAssetDatabase::Initialize()
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

		if (Database.Open(*DBPath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
		{
			CreateTables();

			Database.Execute(TEXT("PRAGMA journal_mode=WAL;"));
			Database.Execute(TEXT("PRAGMA synchronous=NORMAL;"));
			bIsRunning = true;

			// 启动异步工作线程
			AsyncWorker = MakeUnique<FCoDAssetDBWorker>(*this);

			// Database.Execute(
			// 	TEXT("CREATE INDEX IF NOT EXISTS IDX_AssetHashFiles_RelativePath ON AssetHashFiles(Path);"));

			// 处理文件追踪
			ProcessTrackedFiles();
		}
	}
	bIsInitialized = true;
}

void FCoDAssetDatabase::Shutdown()
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

void FCoDAssetDatabase::XSub_Initial(uint64 GameID, const FString& GamePath)
{
	CoDGamePath = GamePath;
	CoDGameHash = GameID;
	FPaths::NormalizeDirectoryName(CoDGamePath);
	XSub_AddGamePath(GameID, GamePath);
	ProcessTrackedFiles();
	// 不会清除已经移除的文件
}

void FCoDAssetDatabase::XSub_AddGamePath(uint64 GameID, const FString& GamePath)
{
	ExecuteStatement(
		TEXT("INSERT OR REPLACE INTO Games (GameHash, GamePath) VALUES (?, ?);"),
		[GameID, &GamePath](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(GameID));
			Stmt.SetBindingValueByIndex(2, GamePath);
			return Stmt.Execute();
		}
	);
}

bool FCoDAssetDatabase::AssetName_InsertOrUpdate(uint64 Hash, const FString& Value)
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

bool FCoDAssetDatabase::AssetName_BatchInsertOrUpdate(const TMap<uint64, FString>& Items)
{
	FScopeLock Lock(&DatabaseCriticalSection);
	if (!Database.IsValid() || Items.Num() == 0) return false;

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

bool FCoDAssetDatabase::AssetName_QueryValue(uint64 Hash, FString& OutValue)
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

bool FCoDAssetDatabase::AssetName_DeleteByHash(uint64 Hash)
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

bool FCoDAssetDatabase::XSub_BatchInsertOrUpdate(const TMap<uint64, FXSubPackageCacheObject>& Items)
{
	FScopeLock Lock(&DatabaseCriticalSection);
	if (!Database.IsValid() || Items.Num() == 0) return false;

	if (!Database.Execute(TEXT("BEGIN TRANSACTION;")))
		return false;

	FSQLitePreparedStatement Statement(
		Database, TEXT(
			"INSERT OR REPLACE INTO SubFileInfo (FileId, DecryptionKey, Offset, CompressedSize, UncompressedSize) VALUES (?, ?, ?, ?, ?);"));
	if (!Statement.IsValid())
	{
		Database.Execute(TEXT("ROLLBACK TRANSACTION;"));
		return false;
	}

	bool bSuccess = true;
	for (const auto& Item : Items)
	{
		Statement.Reset();
		Statement.SetBindingValueByIndex(1, Item.Value.FileId);
		Statement.SetBindingValueByIndex(2, Item.Key);
		Statement.SetBindingValueByIndex(3, Item.Value.Offset);
		Statement.SetBindingValueByIndex(4, Item.Value.CompressedSize);
		Statement.SetBindingValueByIndex(5, Item.Value.UncompressedSize);
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

bool FCoDAssetDatabase::XSub_QueryValue(uint64 Key, FXSubPackageCacheObject& OutValue)
{
	return ExecuteStatement(
		TEXT(R"(
SELECT Sub.Offset, Sub.CompressedSize, Sub.UncompressedSize, 
	(Games.GamePath || '/' || Meta.Path) AS AbsolutePath
		FROM SubFileInfo AS Sub
		INNER JOIN FileMeta AS Meta ON Sub.FileId = Meta.FileId
		INNER JOIN Games ON Meta.GameHash = Games.GameHash
		WHERE Sub.DecryptionKey = ?;
)"),
		[Key, &OutValue](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(Key));
			if (Stmt.Step() == ESQLitePreparedStatementStepResult::Row)
			{
				Stmt.GetColumnValueByIndex(0, OutValue.Offset);
				Stmt.GetColumnValueByIndex(1, OutValue.CompressedSize);
				Stmt.GetColumnValueByIndex(2, OutValue.UncompressedSize);
				Stmt.GetColumnValueByIndex(3, OutValue.Path);
				return true;
			}
			return false;
		}
	);
}

void FCoDAssetDatabase::InsertOrUpdateAsync(uint64 Hash, const FString& Value)
{
	if (AsyncWorker.IsValid())
	{
		AsyncWorker->EnqueueInsertOrUpdate(Hash, Value);
	}
}

void FCoDAssetDatabase::BatchInsertOrUpdateAsync(const TMap<uint64, FString>& Items)
{
	if (AsyncWorker.IsValid())
	{
		AsyncWorker->EnqueueBatchInsertOrUpdate(Items);
	}
}

void FCoDAssetDatabase::QueryValueAsync(uint64 Hash, TFunction<void(const FString&)> Callback)
{
	if (AsyncWorker.IsValid())
	{
		AsyncWorker->EnqueueQuery(Hash, Callback);
	}
}

void FCoDAssetDatabase::DeleteByHashAsync(uint64 Hash)
{
	if (AsyncWorker.IsValid())
	{
		AsyncWorker->EnqueueDelete(Hash);
	}
}

FCoDAssetDatabase::FCoDAssetDatabase(): bIsRunning(false)
{
}

FCoDAssetDatabase::~FCoDAssetDatabase()
{
	Shutdown();
}

void FCoDAssetDatabase::CreateTables()
{
	//  资产名哈希表
	Database.Execute(TEXT(R"(CREATE TABLE IF NOT EXISTS AssetNameCache (
		Hash INTEGER PRIMARY KEY,
		Value TEXT
	);)"));
	// 游戏路径表，插件路径为1（并不会使用插件路径，只是统一格式）
	Database.Execute(TEXT(R"(CREATE TABLE IF NOT EXISTS Games (
	    GameHash INTEGER PRIMARY KEY,   -- 游戏唯一标识符
	    GamePath TEXT UNIQUE NOT NULL   -- 游戏路径
	);)"));
	// 文件追踪表
	Database.Execute(TEXT(R"(CREATE TABLE IF NOT EXISTS FileMeta (
		FileId INTEGER PRIMARY KEY AUTOINCREMENT,
		GameHash INTEGER NOT NULL,  -- 游戏路径对应的哈希，插件哈希始终为1
		Path TEXT UNIQUE,   -- 相对路径
		ContentHash INTEGER,        -- 文件内容的哈希值
		LastModifiedTime INTEGER,   -- 最后修改时间（Unix时间戳）
		FOREIGN KEY (GameHash) REFERENCES Games(GameHash) ON DELETE CASCADE
	);)"));
	// XSub文件信息表
	Database.Execute(TEXT(R"(CREATE TABLE IF NOT EXISTS SubFileInfo (
		DecryptionKey INTEGER PRIMARY KEY,
		Offset INTEGER NOT NULL,
		CompressedSize INTEGER NOT NULL,
		UncompressedSize INTEGER NOT NULL,
		FileId INTEGER NOT NULL,
		FOREIGN KEY (FileId) REFERENCES FileMeta(FileId) ON DELETE CASCADE
	);)"));
}

FString FCoDAssetDatabase::GetDatabasePath() const
{
	return FPaths::ProjectSavedDir() / TEXT("IWToUE") / "AssetName.db";
}

bool FCoDAssetDatabase::ExecuteStatement(const FString& Query,
                                         TFunction<bool(FSQLitePreparedStatement&)> BindAndExecute)
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

void FCoDAssetDatabase::ProcessTrackedFiles()
{
	{
		TArray<FString> WniFiles = FindWniFilesToTrack();
		TMap<uint64, FString> AssetNameMap;
		for (const FString& FilePath : WniFiles)
		{
			FString RelativePath = GetRelativePath(FilePath);
			uint64 ContentHash = ComputeFileContentHash(FilePath);
			int64 LastModifiedTime = GetFileLastModifiedTime(FilePath);

			uint64 ExistingContentHash = 0;
			int64 ExistingLastModifiedTime = 0;

			int64 FileId;
			bool bNeedsUpdate = CheckIfFileNeedsUpdate(1, RelativePath, ContentHash, LastModifiedTime, ExistingContentHash,
													   ExistingLastModifiedTime, FileId);
			if (bNeedsUpdate)
			{
				TMap<uint64, FString> Items;
				ParseWniFile(FilePath, Items);
				AssetNameMap.Append(Items);
				// if (Items.Num() > 0)
				// {
				// 	AssetName_BatchInsertOrUpdate(Items);
				// }
				UpdateTrackedWniFile(FileId, RelativePath, ContentHash, LastModifiedTime);
			}
		}
		if (AssetNameMap.Num() > 0)
		{
			AssetName_BatchInsertOrUpdate(AssetNameMap);
		}
	}
	{
		TArray<FString> XSubFiles = FindXSubFilesToTrack();
		TMap<uint64, FXSubPackageCacheObject> XSubMap;
		for (const FString& FilePath : XSubFiles)
		{
			FString RelativePath = GetRelativePath(FilePath);
			FString NormalizedAbsPath = FPaths::ConvertRelativePathToFull(FilePath);
			FString NormalizedBaseDir = FPaths::ConvertRelativePathToFull(CoDGamePath);
			int32 BaseDirLength = NormalizedBaseDir.Len();
			if (NormalizedAbsPath.Len() > BaseDirLength)
			{
				RelativePath = NormalizedAbsPath.RightChop(BaseDirLength + 1);
			}

			uint64 ContentHash = ComputeFileContentHash(FilePath);
			int64 LastModifiedTime = GetFileLastModifiedTime(FilePath);

			uint64 ExistingContentHash = 0;
			int64 ExistingLastModifiedTime = 0;
			int64 FileId;

			bool bNeedsUpdate = CheckIfFileNeedsUpdate(CoDGameHash, RelativePath, ContentHash, LastModifiedTime,
													   ExistingContentHash, ExistingLastModifiedTime, FileId);
			if (bNeedsUpdate)
			{
				TMap<uint64, FXSubPackageCacheObject> Items;
				ParseXSubFile(FilePath, Items, FileId);
				XSubMap.Append(Items);
				// if (Items.Num() > 0)
				// {
				// 	XSub_BatchInsertOrUpdate(Items);
				// }
				UpdateTrackedXSubFile(FileId, CoDGameHash, RelativePath, ContentHash, LastModifiedTime);
			}
		}
		if (XSubMap.Num() > 0)
		{
			XSub_BatchInsertOrUpdate(XSubMap);
		}
	}
}

TArray<FString> FCoDAssetDatabase::FindWniFilesToTrack() const
{
	TArray<FString> WniFiles;
	FString TrackedDir = FPaths::ProjectPluginsDir() / TEXT("SeImporter");
	IFileManager::Get().FindFilesRecursive(WniFiles, *TrackedDir, TEXT("*.wni"), true, false);

	return WniFiles;
}

TArray<FString> FCoDAssetDatabase::FindXSubFilesToTrack() const
{
	TArray<FString> XSubFiles;
	if (!CoDGamePath.IsEmpty())
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		PlatformFile.FindFilesRecursively(XSubFiles, *CoDGamePath, TEXT(".xsub"));
	}
	return XSubFiles;
}

FString FCoDAssetDatabase::GetRelativePath(const FString& FullPath) const
{
	FString PluginDir = FPaths::ProjectPluginsDir();
	FString RelativePath = FullPath;
	FPaths::MakePathRelativeTo(RelativePath, *PluginDir);
	return RelativePath;
}

uint64 FCoDAssetDatabase::ComputeFileContentHash(const FString& FilePath, int64 ComputeSize) const
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*FilePath));
	if (!FileHandle)
	{
		return 0;
	}

	int64 FileToReadSize = FMath::Min(ComputeSize, FileHandle->Size());
	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(FileToReadSize);
	bool bSuccess = FileHandle->Read(Buffer.GetData(), FileToReadSize);

	if (!bSuccess)
	{
		return 0;
	}

	uint64 Hash = CityHash64((const char*)Buffer.GetData(), FileToReadSize);
	return Hash;
}

int64 FCoDAssetDatabase::GetFileLastModifiedTime(const FString& FilePath) const
{
	FDateTime TimeStamp = IFileManager::Get().GetTimeStamp(*FilePath);
	return TimeStamp.ToUnixTimestamp();
}

void FCoDAssetDatabase::ParseWniFile(const FString& FilePath, TMap<uint64, FString>& Items)
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

void FCoDAssetDatabase::ParseXSubFile(const FString& FilePath, TMap<uint64, FXSubPackageCacheObject>& Items,
                                      int64 FileId)
{
	TUniquePtr<IMappedFileHandle> MappedHandle(
		FPlatformFileManager::Get().GetPlatformFile().OpenMapped(*FilePath));
	if (!MappedHandle)
	{
		return;
	}
	TUniquePtr<IMappedFileRegion> MappedRegion(MappedHandle->MapRegion(0, MappedHandle->GetFileSize()));
	if (!MappedRegion)
	{
		return;
	}
	FLargeMemoryReader Reader((MappedRegion->GetMappedPtr()), MappedRegion->GetMappedSize());

	uint32 Magic = FBinaryReader::ReadUInt32(Reader);
	uint16 Unknown1 = FBinaryReader::ReadUInt16(Reader);
	uint16 Version = FBinaryReader::ReadUInt16(Reader);
	uint64 Unknown = FBinaryReader::ReadUInt64(Reader);
	uint64 Type = FBinaryReader::ReadUInt64(Reader);
	uint64 Size = FBinaryReader::ReadUInt64(Reader);
	FString UnknownHashes = FBinaryReader::ReadFString(Reader, 1896);
	uint64 FileCount = FBinaryReader::ReadUInt64(Reader);
	uint64 DataOffset = FBinaryReader::ReadUInt64(Reader);
	uint64 DataSize = FBinaryReader::ReadUInt64(Reader);
	uint64 HashCount = FBinaryReader::ReadUInt64(Reader);
	uint64 HashOffset = FBinaryReader::ReadUInt64(Reader);
	uint64 HashSize = FBinaryReader::ReadUInt64(Reader);
	uint64 Unknown3 = FBinaryReader::ReadUInt64(Reader);
	uint64 UnknownOffset = FBinaryReader::ReadUInt64(Reader);
	uint64 Unknown4 = FBinaryReader::ReadUInt64(Reader);
	uint64 IndexCount = FBinaryReader::ReadUInt64(Reader);
	uint64 IndexOffset = FBinaryReader::ReadUInt64(Reader);
	uint64 IndexSize = FBinaryReader::ReadUInt64(Reader);

	if (Type != 3) return;
	if (Magic != 0x4950414b || HashOffset >= static_cast<uint64>(Reader.TotalSize()))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid XSub file %s"), *FilePath);
		return;
	}

	Reader.Seek(HashOffset);
	for (uint64 i = 0; i < HashCount; i++)
	{
		uint64 Key = FBinaryReader::ReadUInt64(Reader);
		uint64 PackedInfo = FBinaryReader::ReadUInt64(Reader);
		uint32 PackedInfoEx = FBinaryReader::ReadUInt32(Reader);

		FXSubPackageCacheObject CacheObject;
		CacheObject.Offset = ((PackedInfo >> 32) << 7);
		CacheObject.CompressedSize = ((PackedInfo >> 1) & 0x3FFFFFFF);

		// 解析块头信息计算解压后尺寸
		const int64 OriginalPos = Reader.Tell();
		Reader.Seek(CacheObject.Offset);

		// 读取块头验证Key
		uint16 BlockHeader;
		Reader << BlockHeader;
		uint64 KeyFromBlock;
		Reader << KeyFromBlock;

		if (KeyFromBlock == Key)
		{
			Reader.Seek(CacheObject.Offset + 22);
			uint8 BlockCount;
			Reader << BlockCount;

			uint32 TotalDecompressed = 0;
			for (uint8 j = 0; j < BlockCount; ++j)
			{
				uint8 CompressionType;
				uint32 CompressedSize, DecompressedSize;
				Reader << CompressionType;
				Reader << CompressedSize;
				Reader << DecompressedSize;
				Reader.Seek(12);

				TotalDecompressed += DecompressedSize;
			}
			CacheObject.UncompressedSize = TotalDecompressed;
		}
		else
		{
			CacheObject.UncompressedSize = CacheObject.CompressedSize;
		}

		CacheObject.FileId = FileId;
		
		Reader.Seek(OriginalPos);
		Items.Emplace(Key, CacheObject);
	}
}

bool FCoDAssetDatabase::UpdateTrackedWniFile(int64 FileId, const FString& RelativePath, uint64 ContentHash,
                                             int64 LastModifiedTime)
{
	return ExecuteStatement(
		TEXT(
			"INSERT OR REPLACE INTO FileMeta (FileId, GameHash, Path, ContentHash, LastModifiedTime) VALUES (?, ?, ?, ?, ?);"),
		[&](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, FileId);
			Stmt.SetBindingValueByIndex(2, 1);
			Stmt.SetBindingValueByIndex(3, RelativePath);
			Stmt.SetBindingValueByIndex(4, static_cast<int64>(ContentHash));
			Stmt.SetBindingValueByIndex(5, LastModifiedTime);
			return Stmt.Execute();
		}
	);
}

bool FCoDAssetDatabase::UpdateTrackedXSubFile(int64 FileId, uint64 GameID, const FString& RelativePath, uint64 ContentHash,
                                              int64 LastModifiedTime)
{
	return ExecuteStatement(
		TEXT(
			"INSERT OR REPLACE INTO FileMeta (FileId, GameHash, Path, ContentHash, LastModifiedTime) VALUES (?, ?, ?, ?, ?);"),
		[&](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, FileId);
			Stmt.SetBindingValueByIndex(2, GameID);
			Stmt.SetBindingValueByIndex(3, RelativePath);
			Stmt.SetBindingValueByIndex(4, static_cast<int64>(ContentHash));
			Stmt.SetBindingValueByIndex(5, LastModifiedTime);
			return Stmt.Execute();
		}
	);
}

bool FCoDAssetDatabase::CheckIfFileNeedsUpdate(const uint64 GameHash, const FString& RelativePath,
                                               uint64 NewContentHash, int64 NewLastModifiedTime,
                                               uint64& ExistingContentHash, int64& ExistingLastModifiedTime,
                                               int64& FileId)
{
	bool bExists = ExecuteStatement(
		TEXT("SELECT ContentHash, LastModifiedTime FROM FileMeta WHERE GameHash = ? AND Path = ?;"),
		[&](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, GameHash);
			Stmt.SetBindingValueByIndex(2, RelativePath);
			if (Stmt.Step() == ESQLitePreparedStatementStepResult::Row)
			{
				Stmt.GetColumnValueByIndex(0, ExistingContentHash);
				Stmt.GetColumnValueByIndex(1, ExistingLastModifiedTime);
				return true;
			}
			return false;
		}
	);

	if (!bExists)
	{
		bool bInserted = ExecuteStatement(
			TEXT(
				"INSERT OR REPLACE INTO FileMeta (GameHash, Path, ContentHash, LastModifiedTime) VALUES (?, ?, ?, ?);"),
			[&](FSQLitePreparedStatement& Stmt)
			{
				Stmt.SetBindingValueByIndex(1, GameHash);
				Stmt.SetBindingValueByIndex(2, RelativePath);
				Stmt.SetBindingValueByIndex(3, static_cast<int64>(NewContentHash));
				Stmt.SetBindingValueByIndex(4, NewLastModifiedTime);
				return Stmt.Execute();
			}
		);

		if (bInserted)
		{
			// 获取新插入的记录的ID
			ExecuteStatement(
				TEXT("SELECT last_insert_rowid();"),
				[&](FSQLitePreparedStatement& Stmt)
				{
					if (Stmt.Step() == ESQLitePreparedStatementStepResult::Row)
					{
						Stmt.GetColumnValueByIndex(0, FileId);
					}
					return true;
				}
			);

			ExistingContentHash = NewContentHash;
			ExistingLastModifiedTime = NewLastModifiedTime;
		}
		return true;
	}

	return !bExists || (NewContentHash != ExistingContentHash) || (NewLastModifiedTime != ExistingLastModifiedTime);
}

FOnTaskCompletedDelegate& FCoDAssetDatabase::GetTaskCompletedHandle()
{
	return AsyncWorker->OnTaskCompleted;
}

FCoDAssetDBWorker::FCoDAssetDBWorker(FCoDAssetDatabase& InParent)
	: Parent(InParent), Thread(nullptr), bShouldRun(true)
{
	Thread = FRunnableThread::Create(this, TEXT("KeyValueDatabaseWorker"));
}

FCoDAssetDBWorker::~FCoDAssetDBWorker()
{
	Stop();
	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
	}
}

uint32 FCoDAssetDBWorker::Run()
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
				Parent.AssetName_InsertOrUpdate(Task->Hash, Task->Value);
				break;
			case FDatabaseTask::EType::Query:
				{
					FString Result;
					if (Parent.AssetName_QueryValue(Task->Hash, Result) && Task->Callback)
					{
						AsyncTask(ENamedThreads::GameThread, [Callback = Task->Callback, Result]()
						{
							Callback(Result);
						});
					}
				}
				break;
			case FDatabaseTask::EType::Delete:
				Parent.AssetName_DeleteByHash(Task->Hash);
				break;
			case FDatabaseTask::EType::BatchInsert:
				Parent.AssetName_BatchInsertOrUpdate(Task->BatchItems);
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

void FCoDAssetDBWorker::Stop()
{
	bShouldRun = false;
}

void FCoDAssetDBWorker::EnqueueInsertOrUpdate(uint64 Hash, const FString& Value)
{
	FScopeLock Lock(&QueueCriticalSection);
	auto Task = MakeShared<FDatabaseTask>();
	Task->Type = FDatabaseTask::EType::Insert;
	Task->Hash = Hash;
	Task->Value = Value;
	TaskQueue.Enqueue(Task);
}

void FCoDAssetDBWorker::EnqueueBatchInsertOrUpdate(const TMap<uint64, FString>& Items)
{
	FScopeLock Lock(&QueueCriticalSection);
	auto Task = MakeShared<FDatabaseTask>();
	Task->Type = FDatabaseTask::EType::BatchInsert;
	Task->BatchItems = Items; // 假设TMap支持拷贝，数据量大时需优化
	TaskQueue.Enqueue(Task);
}

void FCoDAssetDBWorker::EnqueueQuery(uint64 Hash, TFunction<void(const FString&)> Callback)
{
	FScopeLock Lock(&QueueCriticalSection);
	auto Task = MakeShared<FDatabaseTask>();
	Task->Type = FDatabaseTask::EType::Query;
	Task->Hash = Hash;
	Task->Callback = Callback;
	TaskQueue.Enqueue(Task);
}

void FCoDAssetDBWorker::EnqueueDelete(uint64 Hash)
{
	FScopeLock Lock(&QueueCriticalSection);
	auto Task = MakeShared<FDatabaseTask>();
	Task->Type = FDatabaseTask::EType::Delete;
	Task->Hash = Hash;
	TaskQueue.Enqueue(Task);
}
