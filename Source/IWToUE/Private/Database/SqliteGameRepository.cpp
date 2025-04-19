#include "Database/SqliteGameRepository.h"

#include "SQLitePreparedStatement.h"
#include "Interface/IDatabaseConnection.h"

bool FSqliteGameRepository::InsertOrUpdateGame(uint64 GameHash, const FString& GamePath)
{
	return Connection->ExecuteStatement(
		TEXT("INSERT OR REPLACE INTO Games (GameHash, GamePath) VALUES (?, ?);"),
		[GameHash, &GamePath](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(GameHash));
			Stmt.SetBindingValueByIndex(2, GamePath);
			return Stmt.Execute();
		}
	);
}

TOptional<FString> FSqliteGameRepository::QueryGamePath(uint64 GameHash)
{
	TOptional<FString> Result;
	Connection->ExecuteStatement(
		TEXT("SELECT GamePath FROM Games WHERE GameHash = ?;"),
		[GameHash, &Result](FSQLitePreparedStatement& Stmt)
		{
			Stmt.SetBindingValueByIndex(1, static_cast<int64>(GameHash));
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
