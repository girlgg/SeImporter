#pragma once

#include "CoreMinimal.h"
#include "SQLiteDatabase.h"
#include "Interface/IDatabaseConnection.h"

/** 
 * @class FSQLiteConnection
 * @brief Manages connections and operations for an SQLite database.
 */
class FSQLiteConnection : public IDatabaseConnection
{
public:
	FSQLiteConnection();
	virtual ~FSQLiteConnection() override;

	/**
	 * @brief Establishes a connection to an SQLite database.
	 * 
	 * @param[in] DatabasePath Filesystem path to SQLite database file
	 * @return true if connection succeeded with full initialization,
	 *         false if any step failed
	 * 
	 * @note Thread-safe through critical section locking
	 * @warning Existing connection will be closed if reopened
	 */
	virtual bool Open(const FString& DatabasePath) override;
	/**
	 * @brief Terminates the active database connection.
	 */
	virtual void Close() override;
	void InternalClose();
	virtual bool IsValid() const override { return Database.IsValid(); }
	virtual bool Execute(const TCHAR* SQL) override;
	virtual bool ExecuteStatement(const FString& Query,
	                              TFunction<bool(FSQLitePreparedStatement&)> BindAndExecute) override;
	virtual bool BeginTransaction() override;
	virtual bool CommitTransaction() override;
	virtual bool RollbackTransaction() override;
	virtual int64 GetLastInsertRowId() override;
	virtual FCriticalSection& GetCriticalSection() override { return DatabaseCriticalSection; }
	virtual FSQLiteDatabase* GetRawDBPtr() override { return &Database; }

private:
	void ApplyOptimizations();
	void CreateTables();

	FSQLiteDatabase Database;
	FCriticalSection DatabaseCriticalSection;
	FString DBPath;
};
