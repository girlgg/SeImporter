#include "CDN/CoDCDNDownloader.h"

void FCoDCDNDownloader::AddFiled(const uint64 CacheID)
{
	FailedAttempts.Add(CacheID, FDateTime::UtcNow());
}

bool FCoDCDNDownloader::HasFailed(const uint64 CacheID)
{
	const FDateTime* FoundTime = FailedAttempts.Find(CacheID);

	if (FoundTime != nullptr)
	{
		FTimespan TimePassed = FDateTime::UtcNow() - *FoundTime;

		if (TimePassed.GetTotalSeconds() <= 60.0)
		{
			return true;
		}
		FailedAttempts.Remove(CacheID);
	}

	return false;
}
