#pragma once

#include <cassert>

#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <string>
#include <tlhelp32.h>
#include <psapi.h>
#include "Windows/HideWindowsPlatformTypes.h"

#include "XSub.h"

class FGameInstance;
struct FCastMapInfo;
class FLargeMemoryReader;

struct FCastMapInfo
{
	FString DisplayName;
	FString DetailInfo;
};

namespace Cordycep
{
	struct XAsset64
	{
		// The asset header.
		uint64 Header;
		// Whether or not this asset is a temp slot.
		uint64 Temp;
		// The next xasset in the list.
		uint64 Next;
		// The previous xasset in the list.
		uint64 Previous;
	};

	struct XAssetPool64
	{
		// The start of the asset chain.
		uint64 Root;
		// The end of the asset chain.
		uint64 End;
		// The asset hash table for this pool.
		uint64 LookupTable;
		// Storage for asset headers for this pool.
		uint64 HeaderMemory;
		// Storage for asset entries for this pool.
		uint64 AssetMemory;
	};
}

class FCordycepProcess
{
public:
	void Initialize();
	FString GetErrorRes();

	FORCEINLINE FString GetCurrentProcessPath() const { return ProcessPath; }
	FORCEINLINE FString GetGameID() const { return GameID; }
	FORCEINLINE uint64 GetPoolsAddress() const { return PoolsAddress; }
	FORCEINLINE uint64 GetStringsAddress() const { return StringsAddress; }
	FORCEINLINE FString GetGameDirectory() const { return GameDirectory; }
	FORCEINLINE FString GetFlags() const { return FString::Join(Flags, TEXT(", ")); }

	TArray<TSharedPtr<FCastMapInfo>> GetMapsInfo();

	void EnumerableAssetPool(uint32 PoolIndex, TFunction<void(const Cordycep::XAsset64&)>&& Action);

	template <typename T>
	T ReadMemory(uint64 Address, bool bIsLocal = false);

	FString ReadFString(uint64 Address);

	void DumpMap(FString MapName);

protected:
	DWORD GetProcessId();
	FString GetProcessPath();
	void* OpenTargetProcess();

	bool IsSinglePlayer();

public:
	// TUniquePtr<FXSub> XSubDecrypt{MakeUnique<FXSub>()};

private:
	void* ProcessHandle{nullptr};

	FGameInstance* GameInstance{nullptr};

	TArray64<uint8> FileDataOld;
	TArray<FString> Flags;

	TUniquePtr<FLargeMemoryReader> StateReader;

	FString ProcessPath;
	FString StateCSI;

	FString GameID;
	FString GameDirectory;

	uint64 PoolsAddress{0};
	uint64 StringsAddress{0};
	int32 GameDirectoryLength{0};

	DWORD ProcessId{0};
};

template <typename T>
T FCordycepProcess::ReadMemory(uint64 Address, bool bIsLocal)
{
	T Result = T();
	if (bIsLocal)
	{
		Result = *reinterpret_cast<T*>(Address);
	}
	else
	{
		static FCriticalSection ProcessHandleCriticalSection;
		FScopeLock Lock(&ProcessHandleCriticalSection);

		if (ProcessHandle)
		{
			SIZE_T BytesRead;
			if (!::ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(Address), &Result, sizeof(T), &BytesRead))
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to read memory from address: 0x%llX"), Address);
				return T();
			}
			assert(BytesRead == sizeof(T));
		}
	}
	return Result;
}
