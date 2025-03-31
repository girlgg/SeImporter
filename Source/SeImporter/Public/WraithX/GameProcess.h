#pragma once

#include <cassert>

#include "CoreMinimal.h"
#include "Structures/CodAssets.h"
#include "UObject/Object.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <string>

#include "AssetNameDB.h"
#include "LocateGameInfo.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "SQLiteDatabase.h"
#include "WraithX/CoDAssetType.h"

struct FCoDAsset;

namespace LocateGameInfo
{
	struct FParasyteBaseState;
}

// Parasytes XAsset Structure.
struct FXAsset64
{
	// The asset header.
	uint64_t Header;
	// Whether or not this asset is a temp slot.
	uint64_t Temp;
	// The next xasset in the list.
	uint64_t Next;
	// The previous xasset in the list.
	uint64_t Previous;
};

// Parasytes XAsset Pool Structure.
struct FXAssetPool64
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

class FGameProcess
{
public:
	FGameProcess();
	~FGameProcess();

	bool IsRunning();

	template <typename T>
	T ReadMemory(uint64 Address, bool bIsLocal = false);
	FString ReadFString(uint64 Address);

	FORCEINLINE TArray<TSharedPtr<FCoDAsset>>& GetLoadedAssets() { return LoadedAssets; }

private:
	bool LocateGameInfo();

	void LoadGameFromParasyte();

	FXAsset64 RequestAsset(const uint64& AssetPtr);

	FString ProcessAssetName(FString Name);

	void ProcessAssetPool(int32 PoolOffset, TFunction<void(FXAsset64)> AssetProcessor);

	void ProcessModelAsset(FXAsset64 AssetNode);
	void ProcessImageAsset(FXAsset64 AssetNode);
	void ProcessAnimAsset(FXAsset64 AssetNode);
	void ProcessMaterialAsset(FXAsset64 AssetNode);
	void ProcessSoundAsset(FXAsset64 AssetNode);

	void AddAssetToCollection(TSharedPtr<FCoDAsset> Asset);
	template <typename TAssetType, typename TCoDType>
	void ProcessGenericAsset(FXAsset64 AssetNode,
	                         const TCHAR* AssetPrefix,
	                         TFunction<void(const TAssetType&, TSharedPtr<TCoDType>)> Customizer);

	void LoadAssets();

	DWORD GetProcessId();
	FString GetProcessPath();
	HANDLE OpenTargetProcess();

	HANDLE ProcessHandle{nullptr};
	FString ProcessPath;
	DWORD ProcessId{0};

	CoDAssets::FCoDGameProcess ProcessInfo{};

	TSharedPtr<LocateGameInfo::FParasyteBaseState> ParasyteBaseState;
	FSQLiteDatabase AssetNameDB;

	TArray<TSharedPtr<FCoDAsset>> LoadedAssets;
	FCriticalSection LoadedAssetsLock;
};

template <typename T>
T FGameProcess::ReadMemory(uint64 Address, bool bIsLocal)
{
	T Result;
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

template <typename TAssetType, typename TCoDType>
void FGameProcess::ProcessGenericAsset(FXAsset64 AssetNode, const TCHAR* AssetPrefix,
                                       TFunction<void(const TAssetType&, TSharedPtr<TCoDType>)> Customizer)
{
	TAssetType Asset = ReadMemory<TAssetType>(AssetNode.Header);
	Asset.Hash &= 0xFFFFFFFFFFFFFFF;

	FAssetNameDB::Get().QueryValueAsync(
		Asset.Hash,
		[this, Asset,Header = AssetNode.Header,Temp = AssetNode.Temp,
			Prefix = FString(AssetPrefix),Customizer = MoveTemp(Customizer)](
		const FString& QueryName)
		{
			TSharedPtr<TCoDType> LoadedAsset = MakeShared<TCoDType>();
			LoadedAsset->AssetName = QueryName.IsEmpty()
				                         ? FString::Format(TEXT("{0}_{1:x}"), {
					                                           Prefix, Asset.Hash
				                                           })
				                         : QueryName;
			LoadedAsset->AssetPointer = Header;
			LoadedAsset->AssetStatus = EWraithAssetStatus::Loaded;

			if constexpr (TIsSame<TCoDType, FCoDAnim>::Value)
			{
				LoadedAsset->AssetStatus = Temp == 1
					                           ? EWraithAssetStatus::Placeholder
					                           : EWraithAssetStatus::Loaded;
			}

			Customizer(Asset, LoadedAsset);
			AddAssetToCollection(LoadedAsset);
		});
}
