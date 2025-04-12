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

#include "CoDAssetDatabase.h"
#include "LocateGameInfo.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "SQLiteDatabase.h"
#include "MapImporter/XSub.h"
#include "WraithX/CoDAssetType.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnOnAssetLoadingDelegate, float);

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
	/*!
	 * @param Address 读取地址
	 * @param OutArray 输出数组
	 * @param Length 读取长度
	 * @return 读取到的字节数
	 */
	template <typename T>
	uint64 ReadArray(uint64 Address, TArray<T>& OutArray, uint64 Length);
	FString ReadFString(uint64 Address);

	FORCEINLINE TArray<TSharedPtr<FCoDAsset>>& GetLoadedAssets() { return LoadedAssets; }

	void LoadGameFromParasyte();

	FOnOnAssetLoadingDelegate OnOnAssetLoadingDelegate;

	FORCEINLINE CoDAssets::ESupportedGames GetCurrentGameType() const { return GameType; }
	FORCEINLINE CoDAssets::ESupportedGameFlags GetCurrentGameFlag() const { return GameFlag; }

	TSharedPtr<FXSub> GetDecrypt() { return XSubDecrypt; }

private:
	bool LocateGameInfo();

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

	void LoadingProgressAdd(float InAddProgress = 1e-6);

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
	FCriticalSection ProgressLock;

	float CurrentLoadingProgress = 0.f;

	TSharedPtr<FXSub> XSubDecrypt;

	CoDAssets::ESupportedGames GameType = CoDAssets::ESupportedGames::None;
	CoDAssets::ESupportedGameFlags GameFlag = CoDAssets::ESupportedGameFlags::None;
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

template <typename T>
uint64 FGameProcess::ReadArray(uint64 Address, TArray<T>& OutArray, uint64 Length)
{
	uint64 ReadSize = 0;
	static FCriticalSection ProcessHandleCriticalSection;
	FScopeLock Lock(&ProcessHandleCriticalSection);

	OutArray.SetNumUninitialized(Length);

	if (ProcessHandle)
	{
		uint64 TotalBytesToRead = Length * sizeof(T);

		ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(Address), OutArray.GetData(), TotalBytesToRead,
		                  &ReadSize);
		assert(ReadSize == TotalBytesToRead);
	}
	return ReadSize;
}

template <typename TAssetType, typename TCoDType>
void FGameProcess::ProcessGenericAsset(FXAsset64 AssetNode, const TCHAR* AssetPrefix,
                                       TFunction<void(const TAssetType&, TSharedPtr<TCoDType>)> Customizer)
{
	TAssetType Asset = ReadMemory<TAssetType>(AssetNode.Header);
	Asset.Hash &= 0xFFFFFFFFFFFFFFF;

	FCoDAssetDatabase::Get().QueryValueAsync(
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
			LoadingProgressAdd();
		});
}
