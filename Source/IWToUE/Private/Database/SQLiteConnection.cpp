#include "Database/SQLiteConnection.h"

#include "SeLogChannels.h"

FSQLiteConnection::FSQLiteConnection()
{
}

FSQLiteConnection::~FSQLiteConnection()
{
}

bool FSQLiteConnection::Open(const FString& DatabasePath)
{
	FScopeLock Lock(&DatabaseCriticalSection);
	if (Database.IsValid()) { InternalClose(); }

	DBPath = DatabasePath;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString DBDir = FPaths::GetPath(DBPath);
	if (!PlatformFile.DirectoryExists(*DBDir))
	{
		if (!PlatformFile.CreateDirectoryTree(*DBDir))
		{
			UE_LOG(LogITUDatabase, Error, TEXT("Failed to create database directory: %s"), *DBDir);
			return false;
		}
	}
	if (Database.Open(*DBPath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		ApplyOptimizations();
		CreateTables();
		UE_LOG(LogITUDatabase, Log, TEXT("Database connection opened: %s"), *DBPath);
		return true;
	}
	UE_LOG(LogITUDatabase, Error, TEXT("Failed to open database: %s"), *Database.GetLastError());
	return false;
}

void FSQLiteConnection::Close()
{
	FScopeLock Lock(&DatabaseCriticalSection);
	InternalClose();
}

void FSQLiteConnection::InternalClose()
{
	if (Database.IsValid())
	{
		Database.Close();
		UE_LOG(LogITUDatabase, Log, TEXT("Database connection closed: %s"), *DBPath);
	}
}

bool FSQLiteConnection::Execute(const TCHAR* SQL)
{
	FScopeLock Lock(&DatabaseCriticalSection);
	if (!Database.IsValid()) return false;
	return Database.Execute(SQL);
}

bool FSQLiteConnection::ExecuteStatement(const FString& Query,
                                         TFunction<bool(FSQLitePreparedStatement&)> BindAndExecute)
{
	FScopeLock Lock(&DatabaseCriticalSection);
	if (!Database.IsValid()) return false;

	FSQLitePreparedStatement Statement(Database, *Query);
	if (Statement.IsValid())
	{
		return BindAndExecute(Statement);
	}
	UE_LOG(LogTemp, Error, TEXT("Failed to prepare statement: %s. Query: %s"), *Database.GetLastError(), *Query);
	return false;
}

bool FSQLiteConnection::BeginTransaction()
{
	return Execute(TEXT("BEGIN TRANSACTION;"));
}

bool FSQLiteConnection::CommitTransaction()
{
	return Execute(TEXT("COMMIT TRANSACTION;"));
}

bool FSQLiteConnection::RollbackTransaction()
{
	return Execute(TEXT("ROLLBACK TRANSACTION;"));
}

int64 FSQLiteConnection::GetLastInsertRowId()
{
	FScopeLock Lock(&DatabaseCriticalSection);
	if (!Database.IsValid()) return 0;
	return Database.GetLastInsertRowId();
}

void FSQLiteConnection::ApplyOptimizations()
{
	Database.Execute(TEXT("PRAGMA journal_mode = WAL;")); // 更高的读取速度
	Database.Execute(TEXT("PRAGMA synchronous = NORMAL;")); // 更快写入（非断电安全）
	Database.Execute(TEXT("PRAGMA cache_size = -10000;")); // 10MB cache
	Database.Execute(TEXT("PRAGMA temp_store = MEMORY;"));
	Database.Execute(TEXT("PRAGMA locking_mode = NORMAL;"));
}

void FSQLiteConnection::CreateTables()
{
	//  资产名哈希表
	Execute(TEXT(R"(CREATE TABLE IF NOT EXISTS AssetNameCache (
		Hash INTEGER PRIMARY KEY,
		Value TEXT
	);)"));
	// 游戏路径表，插件路径为1（并不会使用插件路径，只是统一格式）
	Execute(TEXT(R"(CREATE TABLE IF NOT EXISTS Games (
	    GameHash INTEGER PRIMARY KEY,
	    GamePath TEXT UNIQUE NOT NULL
	);)"));
	// 文件追踪表
	Execute(TEXT(R"(CREATE TABLE IF NOT EXISTS FileMeta (
		FileId INTEGER PRIMARY KEY AUTOINCREMENT,
		GameHash INTEGER NOT NULL,
		Path TEXT NOT NULL, -- Relative Path
		ContentHash INTEGER,
		LastModifiedTime INTEGER,
		UNIQUE(GameHash, Path), -- Ensure unique path per game
		FOREIGN KEY (GameHash) REFERENCES Games(GameHash) ON DELETE CASCADE
	);)"));
	Execute(TEXT("CREATE INDEX IF NOT EXISTS idx_filemeta_path ON FileMeta(GameHash, Path);"));
	// XSub文件信息表
	Execute(TEXT(R"(CREATE TABLE IF NOT EXISTS SubFileInfo (
		DecryptionKey INTEGER PRIMARY KEY,
		Offset INTEGER NOT NULL,
		CompressedSize INTEGER NOT NULL,
		UncompressedSize INTEGER NOT NULL,
		FileId INTEGER NOT NULL,
		FOREIGN KEY (FileId) REFERENCES FileMeta(FileId) ON DELETE CASCADE
	);)"));
	Execute(TEXT("CREATE INDEX IF NOT EXISTS idx_subfileinfo_fileid ON SubFileInfo(FileId);"));
}
