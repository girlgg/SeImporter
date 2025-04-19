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

class IGameAssetDiscoverer;
class IMemoryReader;
class FCoDCDNDownloader;

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

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssetLoadingProgressDelegate, float);
DECLARE_MULTICAST_DELEGATE(FOnAssetLoadingCompleteDelegate);

class FGameProcess : public TSharedFromThis<FGameProcess>
{
public:
	FGameProcess();
	~FGameProcess();

	// --- Initialization and Status ---
	// Finds process, creates reader/discoverer
	bool Initialize();
	bool IsInitialized() const { return bIsInitialized; }
	bool IsGameRunning();

	// --- Asset Loading ---
	void StartAssetDiscovery();
	bool IsDiscoveringAssets() const { return bIsDiscovering; }
	void SetTotalAssetCnt(int32 InCnt) { TotalDiscoveredAssets = InCnt; }

	FORCEINLINE TArray<TSharedPtr<FCoDAsset>>& GetLoadedAssets() { return LoadedAssets; }

	FORCEINLINE CoDAssets::ESupportedGames GetCurrentGameType() const;
	FORCEINLINE CoDAssets::ESupportedGameFlags GetCurrentGameFlag() const;
	FORCEINLINE FCoDCDNDownloader* GetCDNDownloader();
	TSharedPtr<FXSub> GetDecrypt();

	// --- Delegates ---
	// Renamed delegate for clarity
	FOnAssetLoadingProgressDelegate OnAssetLoadingProgress;
	FOnAssetLoadingCompleteDelegate OnAssetLoadingComplete;

private:
	// --- Helper Methods ---
	bool FindTargetProcess();
	bool OpenProcessHandleAndReader();
	bool LocateGameInfoViaParasyte();
	bool CreateAndInitializeDiscoverer();

	// --- Callbacks ---
	void HandleAssetDiscovered(TSharedPtr<FCoDAsset> DiscoveredAsset);
	void HandleDiscoveryComplete();

	bool LocateGameInfo();

	void ProcessModelAsset(FXAsset64 AssetNode);
	void ProcessImageAsset(FXAsset64 AssetNode);
	void ProcessAnimAsset(FXAsset64 AssetNode);
	void ProcessMaterialAsset(FXAsset64 AssetNode);
	void ProcessSoundAsset(FXAsset64 AssetNode);

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
	TUniquePtr<FCoDCDNDownloader> CDNDownloader;

	CoDAssets::ESupportedGames GameType = CoDAssets::ESupportedGames::None;
	CoDAssets::ESupportedGameFlags GameFlag = CoDAssets::ESupportedGameFlags::None;

	// --- Internal State ---
	DWORD TargetProcessId = 0;
	CoDAssets::FCoDGameProcess TargetProcessInfo{};
	FString TargetProcessPath;
	TSharedPtr<LocateGameInfo::FParasyteBaseState> ParasyteState;
	TSharedPtr<IGameAssetDiscoverer> AssetDiscoverer;

	TSharedPtr<IMemoryReader> MemoryReader;

	float TotalDiscoveredAssets = 0.0f;
	float CurrentDiscoveryProgressCount = 0.0f;

	// --- Async Task Management ---
	FAsyncTask<class FAssetDiscoveryTask>* DiscoveryTask = nullptr;

	std::atomic<bool> bIsInitialized = false;
	std::atomic<bool> bIsDiscovering = false;

	friend class FAssetDiscoveryTask;
};


/*
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
*/
