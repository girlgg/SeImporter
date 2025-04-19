#pragma once

#include "Interface/DatabaseRepository.h"

class IDatabaseConnection;

class FSqliteGameRepository final : public IGameRepository
{
public:
	FSqliteGameRepository(TSharedRef<IDatabaseConnection> InConnection) : Connection(InConnection)
	{
	}

	virtual bool InsertOrUpdateGame(uint64 GameHash, const FString& GamePath) override;
	virtual TOptional<FString> QueryGamePath(uint64 GameHash) override;

private:
	TSharedRef<IDatabaseConnection> Connection;
};
