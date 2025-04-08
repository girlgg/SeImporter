#include "WraithX/GameProcess.h"

#include "Structures/MW6GameStructures.h"
#include "WraithX/CoDAssetDatabase.h"
#include "WraithX/LocateGameInfo.h"

FGameProcess::FGameProcess()
{
	ProcessId = GetProcessId();
	ProcessHandle = OpenTargetProcess();
	ProcessPath = GetProcessPath();

	LoadingProgressAdd(0.1f);

	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		if (LocateGameInfo())
		{
			if (ParasyteBaseState.IsValid() && ParasyteBaseState->GameID != 0)
			{
				LoadGameFromParasyte();
			}
		}
	});
}

FGameProcess::~FGameProcess()
{
	if (ProcessHandle)
		CloseHandle(ProcessHandle);
	ProcessHandle = nullptr;
	FCoDAssetDatabase::Get().Shutdown();
}

bool FGameProcess::IsRunning()
{
	if (ProcessHandle != NULL)
	{
		DWORD Result = WaitForSingleObject(ProcessHandle, 0);
		if (Result == WAIT_FAILED)
		{
			DWORD lastError = GetLastError();
			UE_LOG(LogTemp, Error, TEXT("Wait failed with error: %ld"), lastError);
		}
		return Result == WAIT_TIMEOUT;
	}
	return false;
}


uint64 FGameProcess::ReadArray(uint64 Address, TArray<uint8>& OutArray, uint64 Length)
{
	uint64 ReadSize = 0;
	static FCriticalSection ProcessHandleCriticalSection;
	FScopeLock Lock(&ProcessHandleCriticalSection);

	OutArray.SetNum(Length);

	if (ProcessHandle)
	{
		ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(Address), OutArray.GetData(), Length, &ReadSize);
		assert(BytesRead == sizeof(T));
	}
	return ReadSize;
}

FString FGameProcess::ReadFString(uint64 Address)
{
	TArray<uint8> Data;
	while (true)
	{
		uint8 Tmp = ReadMemory<uint8>(Address);
		if (!Tmp) break;
		Data.Add(Tmp);
		Address += 1;
	}
	Data.Add(0);

	return FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(Data.GetData())));
}

bool FGameProcess::LocateGameInfo()
{
	switch (ProcessInfo.GameID)
	{
	case CoDAssets::ESupportedGames::Parasyte:
		return LocateGameInfo::Parasyte(ProcessPath, ParasyteBaseState);
	default:
		break;
	}
	return false;
}

void FGameProcess::LoadGameFromParasyte()
{
	if (!IsRunning())
	{
		return;
	}
	switch (ParasyteBaseState->GameID)
	{
	// Modern Warfare 3 (2023)
	case 0x4B4F4D41594D4159:
		GameType = CoDAssets::ESupportedGames::ModernWarfare6;
		XSubDecrypt = MakeShared<FXSub>(ParasyteBaseState->GameID, ParasyteBaseState->GameDirectory);
		LoadAssets();
		break;
	}
}

FXAsset64 FGameProcess::RequestAsset(const uint64& AssetPtr)
{
	return ReadMemory<FXAsset64>(AssetPtr);
}

FString FGameProcess::ProcessAssetName(FString Name)
{
	Name = Name.Replace(TEXT("::"), TEXT("_")).TrimStartAndEnd();

	if (int32 Index; Name.FindLastChar(TEXT('/'), Index))
	{
		Name = Name.RightChop(Index + 1);
	}

	if (int32 Index; Name.FindLastChar(TEXT(':'), Index) &&
		Index > 0 && Name[Index - 1] == ':')
	{
		Name = Name.RightChop(Index + 1);
	}

	return Name;
}

void FGameProcess::ProcessAssetPool(int32 PoolOffset, TFunction<void(FXAsset64)> AssetProcessor)
{
	const auto Pool = ReadMemory<FXAssetPool64>(ParasyteBaseState->PoolsAddress + sizeof(FXAssetPool64) * PoolOffset);

	for (auto AssetNode = RequestAsset(Pool.Root); ; AssetNode = RequestAsset(AssetNode.Next))
	{
		if (AssetNode.Header)
		{
			AssetProcessor(AssetNode);
		}
		if (!AssetNode.Next) break;
	}
}

void FGameProcess::ProcessModelAsset(FXAsset64 AssetNode)
{
	FMW6XModel Model = ReadMemory<FMW6XModel>(AssetNode.Header);
	Model.Hash &= 0xFFFFFFFFFFFFFFF;

	auto CreateModel = [&](const FString& ModelName)
	{
		TSharedPtr<FCoDModel> LoadedModel = MakeShared<FCoDModel>();
		LoadedModel->AssetType = EWraithAssetType::Model;
		LoadedModel->AssetName = ModelName;
		LoadedModel->AssetPointer = AssetNode.Header;
		LoadedModel->BoneCount = (Model.ParentListPtr > 0) * (Model.NumBones + Model.UnkBoneCount);
		LoadedModel->LodCount = Model.NumLods;
		LoadedModel->AssetStatus = AssetNode.Temp == 1
			                           ? EWraithAssetStatus::Placeholder
			                           : EWraithAssetStatus::Loaded;

		AddAssetToCollection(LoadedModel);
	};

	if (Model.NamePtr)
	{
		CreateModel(ProcessAssetName(ReadFString(Model.NamePtr)));
	}
	else
	{
		FCoDAssetDatabase::Get().QueryValueAsync(Model.Hash, [=](const FString& QueryName)
		{
			CreateModel(QueryName.IsEmpty()
				            ? FString::Printf(TEXT("xmodel_%llx"), Model.Hash)
				            : QueryName);
		});
	}
}

void FGameProcess::ProcessImageAsset(FXAsset64 AssetNode)
{
	ProcessGenericAsset<FMW6GfxImage, FCoDImage>(AssetNode, TEXT("ximage"),
	                                             [](auto& Image, auto LoadedImage)
	                                             {
		                                             LoadedImage->AssetType = EWraithAssetType::Image;
		                                             LoadedImage->Width = Image.Width;
		                                             LoadedImage->Height = Image.Height;
		                                             LoadedImage->Format = Image.ImageFormat;
		                                             LoadedImage->Streamed = Image.LoadedImagePtr == 0;
	                                             });
}

void FGameProcess::ProcessAnimAsset(FXAsset64 AssetNode)
{
	ProcessGenericAsset<FMW6XAnim, FCoDAnim>(AssetNode, TEXT("xanim"),
	                                         [](auto& Anim, auto LoadedAnim)
	                                         {
		                                         LoadedAnim->AssetType = EWraithAssetType::Animation;
		                                         LoadedAnim->Framerate = Anim.Framerate;
		                                         LoadedAnim->FrameCount = Anim.FrameCount;
		                                         LoadedAnim->BoneCount = Anim.TotalBoneCount;
	                                         });
}

void FGameProcess::ProcessMaterialAsset(FXAsset64 AssetNode)
{
	ProcessGenericAsset<FMW6Material, FCoDMaterial>(AssetNode, TEXT("xmaterial"),
	                                                [](auto& Material, auto LoadedMaterial)
	                                                {
		                                                LoadedMaterial->AssetType = EWraithAssetType::Material;
		                                                LoadedMaterial->ImageCount = Material.ImageCount;
	                                                });
}

void FGameProcess::ProcessSoundAsset(FXAsset64 AssetNode)
{
	ProcessGenericAsset<FMW6SoundAsset, FCoDSound>(AssetNode, TEXT("xmaterial"),
	                                               [](auto& SoundAsset, auto LoadedSound)
	                                               {
		                                               LoadedSound->FullName = LoadedSound->AssetName;
		                                               LoadedSound->AssetType = EWraithAssetType::Sound;
		                                               LoadedSound->AssetName = FPaths::GetBaseFilename(
			                                               LoadedSound->AssetName);
		                                               int32 DotIndex = LoadedSound->AssetName.Find(TEXT("."));
		                                               if (DotIndex != INDEX_NONE)
		                                               {
			                                               LoadedSound->AssetName = LoadedSound->AssetName.Left(
				                                               DotIndex);
		                                               }
		                                               LoadedSound->FullPath = FPaths::GetPath(LoadedSound->AssetName);

		                                               LoadedSound->FrameRate = SoundAsset.FrameRate;
		                                               LoadedSound->FrameCount = SoundAsset.FrameCount;
		                                               LoadedSound->ChannelsCount = SoundAsset.ChannelCount;
		                                               LoadedSound->AssetSize = -1;
		                                               LoadedSound->bIsFileEntry = false;
		                                               LoadedSound->Length = 1000.0f * (LoadedSound->FrameCount /
			                                               static_cast<float>(LoadedSound->FrameRate));
	                                               });
}

void FGameProcess::AddAssetToCollection(TSharedPtr<FCoDAsset> Asset)
{
	if (&LoadedAssetsLock)
	{
		FScopeLock Lock(&LoadedAssetsLock);
		LoadedAssets.Add(MoveTemp(Asset));
	}
}

void FGameProcess::LoadingProgressAdd(float InAddProgress)
{
	FScopeLock Lock(&ProgressLock);

	CurrentLoadingProgress += InAddProgress;
	if (CurrentLoadingProgress > 0.01f)
	{
		float ProgressToSend = CurrentLoadingProgress;
		CurrentLoadingProgress = 0;
		AsyncTask(ENamedThreads::GameThread, [ProgressToSend, this]
		{
			if (this)
			{
				OnOnAssetLoadingDelegate.Broadcast(ProgressToSend);
			}
		});
	}
}

void FGameProcess::LoadAssets()
{
	struct FAssetPoolInfo
	{
		int32 Offset;
		TFunction<void(FXAsset64)> Processor;
	};

	TArray<FAssetPoolInfo> AssetPools = {
		{9, [this](FXAsset64 AssetNode) { ProcessModelAsset(AssetNode); }},
		{21, [this](FXAsset64 AssetNode) { ProcessImageAsset(AssetNode); }},
		{7, [this](FXAsset64 AssetNode) { ProcessAnimAsset(AssetNode); }},
		{11, [this](FXAsset64 AssetNode) { ProcessMaterialAsset(AssetNode); }},
		{0xc1, [this](FXAsset64 AssetNode) { ProcessSoundAsset(AssetNode); }}
	};

	for (int32 PoolIdx = 0; PoolIdx < AssetPools.Num(); ++PoolIdx)
	{
		const auto& [Offset, Processor] = AssetPools[PoolIdx];
		ProcessAssetPool(Offset, Processor);
	}
}

DWORD FGameProcess::GetProcessId()
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe32))
		{
			do
			{
				for (const auto& GameProcessInfo : CoDAssets::GameProcessInfos)
				{
					if (std::wstring ProcessName = *GameProcessInfo.ProcessName; ProcessName == pe32.szExeFile)
					{
						ProcessInfo = GameProcessInfo;
						return pe32.th32ProcessID;
					}
				}
			}
			while (Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}
	return 0;
}

FString FGameProcess::GetProcessPath()
{
	wchar_t path[MAX_PATH];
	if (GetModuleFileNameEx(ProcessHandle, NULL, path, MAX_PATH))
	{
		return FString(path);
	}
	return "";
}

void* FGameProcess::OpenTargetProcess()
{
	return OpenProcess(PROCESS_ALL_ACCESS, false, ProcessId);
}
