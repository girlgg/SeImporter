#pragma once
#include "SQLitePreparedStatement.h"

class FSQLiteDatabase;

class IDatabaseConnection : public FNoncopyable
{
public:
	virtual ~IDatabaseConnection() = default;
	virtual bool Open(const FString& DatabasePath) = 0;
	virtual void Close() = 0;
	virtual bool IsValid() const = 0;
	virtual bool Execute(const TCHAR* SQL) = 0;
	virtual bool ExecuteStatement(const FString& Query, TFunction<bool(FSQLitePreparedStatement&)> BindAndExecute) = 0;
	virtual bool BeginTransaction() = 0;
	virtual bool CommitTransaction() = 0;
	virtual bool RollbackTransaction() = 0;
	virtual int64 GetLastInsertRowId() = 0;
	virtual FCriticalSection& GetCriticalSection() = 0;
	virtual FSQLiteDatabase* GetRawDBPtr() = 0;
};
