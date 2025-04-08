#include "MapImporter/CordycepProcess.h"

#include "GameInfo/BlackOps6.h"
#include "GameInfo/ModernWarfare6.h"
#include "GameInfo/ModernWarfare6SP.h"
#include "MapImporter/XSub.h"

#include "Serialization/LargeMemoryReader.h"
#include "Structures/BO6GameStructures.h"
#include "Structures/MW6GameStructures.h"
#include "Structures/MW6SPGameStructures.h"
#include "Utils/BinaryReader.h"
#include "MapImporter/CASCPackage.h"

void FCordycepProcess::Initialize()
{
	ProcessId = GetProcessId();
	ProcessHandle = OpenTargetProcess();
	ProcessPath = GetProcessPath();

	StateCSI = FPaths::GetPath(ProcessPath) / FString(TEXT("Data")) / FString(TEXT("CurrentHandler.csi"));
	if (FPaths::FileExists(StateCSI))
	{
		if (FFileHelper::LoadFileToArray(FileDataOld, *StateCSI))
		{
			StateReader = MakeUnique<FLargeMemoryReader>(FileDataOld.GetData(), FileDataOld.Num());
		}
	}
	if (StateReader.IsValid())
	{
		GameID = FBinaryReader::ReadFString(*StateReader, 8);
		PoolsAddress = FBinaryReader::ReadUInt64(*StateReader);
		StringsAddress = FBinaryReader::ReadUInt64(*StateReader);
		GameDirectoryLength = FBinaryReader::ReadInt32(*StateReader);
		GameDirectory = FBinaryReader::ReadFString(*StateReader, GameDirectoryLength);

		const uint32 FlagsCount = FBinaryReader::ReadUInt32(*StateReader);
		for (uint32 i = 0; i < FlagsCount; i++)
		{
			const int32 FlagLength = FBinaryReader::ReadInt32(*StateReader);
			FString Flag = FBinaryReader::ReadFString(*StateReader, FlagLength);
			Flags.Add(Flag);
		}
	}
	// FCASCPackage::LoadFiles(GameDirectory);
	// XSubDecrypt->LoadFilesAsync(GameDirectory);
	if (GameID == "YAMYAMOK")
	{
		if (IsSinglePlayer())
		{
			GameInstance = new FModernWarfare6SP(this);
		}
		else
		{
			GameInstance = new FModernWarfare6(this);
		}
	}
	else if (GameID == "BLACKOP6")
	{
		GameInstance = new FBlackOps6(this);
	}
}

FString FCordycepProcess::GetErrorRes()
{
	if (ProcessId <= 0)
	{
		return TEXT("Cordycep is not running!");
	}
	if (ProcessHandle == nullptr)
	{
		return TEXT("Failed to open Cordycep process!");
	}
	if (ProcessPath.IsEmpty())
	{
		return TEXT("Failed to get Cordycep process path!");
	}
	if (StateCSI.IsEmpty())
	{
		return TEXT("Failed to find CurrentHandler.csi!");
	}
	if (!StateReader.IsValid())
	{
		return TEXT("Failed to read CurrentHandler.csi!");
	}
	if (GameID != "YAMYAMOK" && GameID != "BLACKOP6")
	{
		return TEXT("Unsupported game!");
	}
	return TEXT("");
}

TArray<TSharedPtr<FCastMapInfo>> FCordycepProcess::GetMapsInfo()
{
	TArray<TSharedPtr<FCastMapInfo>> MapList;
	if (GameInstance)
	{
		MapList = GameInstance->GetMapsInfo();
	}
	return MapList;
}

FString FCordycepProcess::ReadFString(uint64 Address)
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

void FCordycepProcess::DumpMap(FString MapName)
{
	if (GameInstance)
	{
		GameInstance->DumpMap(MapName);
	}
}

DWORD FCordycepProcess::GetProcessId()
{
	std::wstring processName = L"Cordycep.CLI.exe";
	DWORD processId = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe32))
		{
			do
			{
				if (processName == pe32.szExeFile)
				{
					processId = pe32.th32ProcessID;
					break;
				}
			}
			while (Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}
	return processId;
}

FString FCordycepProcess::GetProcessPath()
{
	wchar_t path[MAX_PATH];
	if (GetModuleFileNameEx(ProcessHandle, NULL, path, MAX_PATH))
	{
		return FString(path);
	}
	return "";
}

void* FCordycepProcess::OpenTargetProcess()
{
	return OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, false, ProcessId);
}

bool FCordycepProcess::IsSinglePlayer()
{
	return Flags.Contains(TEXT("sp"));
}

void FCordycepProcess::EnumerableAssetPool(uint32 PoolIndex, TFunction<void(const Cordycep::XAsset64&)>&& Action)
{
	const uintptr_t PoolPtr = PoolsAddress + PoolIndex * sizeof(Cordycep::XAssetPool64);
	const Cordycep::XAssetPool64 Pool = ReadMemory<Cordycep::XAssetPool64>(PoolPtr);
	uint64 CurrentAssetPtr = Pool.Root;
	while (CurrentAssetPtr != 0)
	{
		const Cordycep::XAsset64 asset = ReadMemory<Cordycep::XAsset64>(CurrentAssetPtr);
		if (asset.Header != 0)
		{
			Action(asset);
		}
		CurrentAssetPtr = asset.Next;
	}
}
