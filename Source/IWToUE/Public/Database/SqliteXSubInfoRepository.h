#pragma once
#include "Interface/DatabaseRepository.h"

class IDatabaseConnection;

class FSqliteXSubInfoRepository : public IXSubInfoRepository
{
public:
	FSqliteXSubInfoRepository(TSharedRef<IDatabaseConnection> InConnection) : Connection(InConnection)
	{
	}

	virtual bool BatchInsertOrUpdate(const TMap<uint64, FXSubPackageCacheObject>& Items) override;
	virtual TOptional<FXSubPackageCacheObject> QueryValue(uint64 DecryptionKey) override;

private:
	TSharedRef<IDatabaseConnection> Connection;
};
