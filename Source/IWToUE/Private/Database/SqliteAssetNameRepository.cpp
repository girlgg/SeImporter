#include "Database/SqliteAssetNameRepository.h"

#include "SQLiteDatabase.h"
#include "SQLitePreparedStatement.h"
#include "Interface/IDatabaseConnection.h"

bool FSqliteAssetNameRepository::InsertOrUpdate(uint64 Hash, const FString& Value)
{
	return Connection->ExecuteStatement(
		TEXT("INSERT OR REPLACE INTO AssetNameCache (Hash, Value) VALUES (?, ?);"),
		[Hash, &Value](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(Hash));
			Stmt.SetBindingValueByIndex(2, Value);
			return Stmt.Execute();
		}
	);
}

bool FSqliteAssetNameRepository::BatchInsertOrUpdate(const TMap<uint64, FString>& Items)
{
	if (Items.Num() == 0) return true;
	if (!Connection->BeginTransaction()) return false;
	FSQLitePreparedStatement Statement(*Connection->GetRawDBPtr(),
	                                   TEXT("INSERT OR REPLACE INTO AssetNameCache (Hash, Value) VALUES (?, ?);"));
	if (!Statement.IsValid())
	{
		Connection->RollbackTransaction();
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

TOptional<FString> FSqliteAssetNameRepository::QueryValue(uint64 Hash)
{
	TOptional<FString> Result;
	Connection->ExecuteStatement(
		TEXT("SELECT Value FROM AssetNameCache WHERE Hash = ?;"),
		[Hash, &Result](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(Hash));
			if (Stmt.Step() == ESQLitePreparedStatementStepResult::Row)
			{
				FString Value;
				Stmt.GetColumnValueByIndex(0, Value);
				Result.Emplace(MoveTemp(Value));
				return true;
			}
			return false;
		}
	);
	return Result;
}

bool FSqliteAssetNameRepository::DeleteByHash(uint64 Hash)
{
	return Connection->ExecuteStatement(
		TEXT("DELETE FROM AssetNameCache WHERE Hash = ?;"),
		[Hash](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(Hash));
			return Stmt.Execute();
		}
	);
}
