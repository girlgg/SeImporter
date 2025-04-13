#pragma once

#include "CoreMinimal.h"

struct FDownloadMemoryResult
{
	TArray<uint8> DataBuffer;

	FString AsString() const
	{
		return FString(DataBuffer.Num(), (const ANSICHAR*)DataBuffer.GetData());
	}

	bool SaveToFile(const FString& FilePath) const
	{
		return FFileHelper::SaveArrayToFile(DataBuffer, *FilePath);
	}
};

class IWebClient
{
public:
	virtual ~IWebClient() = default;

	/*!
	 * 异步下载
	 * @param Url 
	 * @param Callback 
	 */
	virtual void DownloadData(const FString& Url, TFunction<void(TSharedPtr<FDownloadMemoryResult>)> Callback) = 0;

	/*!
	 * 同步下载
	 * @param Url 
	 * @return 
	 */
	virtual TSharedPtr<FDownloadMemoryResult> DownloadData(const FString& Url) = 0;
};
