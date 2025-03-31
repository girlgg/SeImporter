#pragma once

namespace LocateGameInfo
{
	struct FParasyteBaseState
	{
		uint64 GameID{0};
		FString GameDirectory;
		uint64 PoolsAddress{0};
		uint64 StringsAddress{0};
		TArray<FString> Flags;
	};
	
	bool Parasyte(const FString & ProcessPath, TSharedPtr<FParasyteBaseState> & OutState);
}
