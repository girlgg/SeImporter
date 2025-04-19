#pragma once

class IFileTracker
{
public:
	virtual ~IFileTracker() = default;
	virtual void Initialize(uint64 GameHash, const FString& GamePath) = 0;
	virtual void StartTrackingAsync() = 0;
	virtual bool IsTrackingComplete() const = 0;

	DECLARE_DELEGATE(FOnTrackingCompleteDelegate);
	FOnTrackingCompleteDelegate OnComplete;
};
