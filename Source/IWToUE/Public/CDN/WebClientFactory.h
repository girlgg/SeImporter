#pragma once
#include "UEWebClient.h"

class IWebClient;

class FWebClientFactory
{
public:
	static TSharedPtr<IWebClient> Create()
	{
		return MakeShared<FUEWebClient>();
	}
};
