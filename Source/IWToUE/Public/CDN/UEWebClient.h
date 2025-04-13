#pragma once
#include "HttpModule.h"
#include "IWebClient.h"

class FUEWebClient : public IWebClient
{
public:
	FUEWebClient();
	virtual ~FUEWebClient() override = default;

	//~ Begin IWebClient interface
	virtual void DownloadData(const FString& Url, TFunction<void(TSharedPtr<FDownloadMemoryResult>)> Callback) override;
	virtual TSharedPtr<FDownloadMemoryResult> DownloadData(const FString& Url) override;
	//~ End of IWebClient interface

private:
	FHttpModule* HttpModule;

	struct FSyncRequestState
	{
		TSharedPtr<FDownloadMemoryResult> Result;
		bool bIsCompleted = false;
	};

	void HandleAsyncResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful,
	                         TFunction<void(TSharedPtr<FDownloadMemoryResult>)> Callback);
};
