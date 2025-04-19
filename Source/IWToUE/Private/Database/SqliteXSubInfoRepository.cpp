#include "Database/SqliteXSubInfoRepository.h"

#include "SQLiteDatabase.h"
#include "SQLitePreparedStatement.h"
#include "Interface/IDatabaseConnection.h"
#include "MapImporter/XSub.h"

bool FSqliteXSubInfoRepository::BatchInsertOrUpdate(const TMap<uint64, FXSubPackageCacheObject>& Items)
{
	if (Items.Num() == 0) return true;
	if (!Connection->BeginTransaction()) return false;
	FSQLitePreparedStatement Statement(*Connection->GetRawDBPtr(),
	                                   TEXT(
		                                   "INSERT OR REPLACE INTO SubFileInfo (FileId, DecryptionKey, Offset, CompressedSize, UncompressedSize) VALUES (?, ?, ?, ?, ?);"));
	if (!Statement.IsValid())
	{
		Connection->RollbackTransaction();
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
			UE_LOG(LogTemp, Error, TEXT("Batch Insert Failed: %s"), *Connection->GetRawDBPtr()->GetLastError());
			bSuccess = false;
			break;
		}
	}

	if (bSuccess)
		Connection->CommitTransaction();
	else
		Connection->RollbackTransaction();
	return bSuccess;
}

TOptional<FXSubPackageCacheObject> FSqliteXSubInfoRepository::QueryValue(uint64 DecryptionKey)
{
	TOptional<FXSubPackageCacheObject> Result;
	Connection->ExecuteStatement(
		TEXT(R"(SELECT Sub.Offset, Sub.CompressedSize, Sub.UncompressedSize, 
			(Games.GamePath || '/' || Meta.Path) AS AbsolutePath
			FROM SubFileInfo AS Sub
			INNER JOIN FileMeta AS Meta ON Sub.FileId = Meta.FileId
			INNER JOIN Games ON Meta.GameHash = Games.GameHash
			WHERE Sub.DecryptionKey = ?;)"),
		[DecryptionKey, &Result](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(DecryptionKey));
			if (Stmt.Step() == ESQLitePreparedStatementStepResult::Row)
			{
				FXSubPackageCacheObject Value;
				Stmt.GetColumnValueByIndex(0, Value.Offset);
				Stmt.GetColumnValueByIndex(1, Value.CompressedSize);
				Stmt.GetColumnValueByIndex(2, Value.UncompressedSize);
				Stmt.GetColumnValueByIndex(3, Value.Path);
				Result.Emplace(MoveTemp(Value));
				return true;
			}
			return false;
		}
	);
	return Result;
}
