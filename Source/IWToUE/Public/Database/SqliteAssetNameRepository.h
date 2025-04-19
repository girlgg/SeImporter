#pragma once

#include "CoreMinimal.h"
#include "Interface/DatabaseRepository.h"

class IDatabaseConnection;

class FSqliteAssetNameRepository : public IAssetNameRepository
{
public:
	FSqliteAssetNameRepository(TSharedRef<IDatabaseConnection> InConnection) : Connection(InConnection)
	{
	}

	virtual ~FSqliteAssetNameRepository() override = default;

	virtual bool InsertOrUpdate(uint64 Hash, const FString& Value) override;
	virtual bool BatchInsertOrUpdate(const TMap<uint64, FString>& Items) override;
	virtual TOptional<FString> QueryValue(uint64 Hash) override;
	virtual bool DeleteByHash(uint64 Hash) override;

private:
	TSharedRef<IDatabaseConnection> Connection;
};
