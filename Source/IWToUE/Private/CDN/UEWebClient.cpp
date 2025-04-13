#include "CDN/UEWebClient.h"

#include "HttpManager.h"
#include "Interfaces/IHttpResponse.h"

FUEWebClient::FUEWebClient()
{
	HttpModule = &FHttpModule::Get();
}

void FUEWebClient::DownloadData(const FString& Url, TFunction<void(TSharedPtr<FDownloadMemoryResult>)> Callback)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule->CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindRaw(this, &FUEWebClient::HandleAsyncResponse, Callback);
	Request->ProcessRequest();
}

TSharedPtr<FDownloadMemoryResult> FUEWebClient::DownloadData(const FString& Url)
{
	TSharedPtr<FSyncRequestState> RequestState = MakeShared<FSyncRequestState>();

	auto Callback = [RequestState](TSharedPtr<FDownloadMemoryResult> Result)
	{
		RequestState->Result = Result;
		RequestState->bIsCompleted = true;
	};

	DownloadData(Url, Callback);

	// TODO 同步机制
	while (!RequestState->bIsCompleted)
	{
		FPlatformProcess::Sleep(0.01f);
		FHttpModule::Get().GetHttpManager().Tick(0.01f);
	}

	return RequestState->Result;
}

void FUEWebClient::HandleAsyncResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful,
                                       TFunction<void(TSharedPtr<FDownloadMemoryResult>)> Callback)
{
	TSharedPtr<FDownloadMemoryResult> Result;

	if (bWasSuccessful && Response.IsValid())
	{
		Result = MakeShared<FDownloadMemoryResult>();
		Result->DataBuffer.Append(Response->GetContent().GetData(), Response->GetContent().Num());
	}

	if (IsInGameThread())
	{
		Callback(Result);
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [Callback, Result]()
		{
			Callback(Result);
		});
	}
}
