#include "WraithX/LocateGameInfo.h"

#include "Serialization/LargeMemoryReader.h"
#include "Utils/BinaryReader.h"

bool LocateGameInfo::Parasyte(const FString& ProcessPath, TSharedPtr<FParasyteBaseState>& OutState)
{
	if (OutState.IsValid())
	{
		OutState.Reset();
	}
	
	FString StateCSIPath = FPaths::GetPath(ProcessPath) / FString(TEXT("Data")) / FString(TEXT("CurrentHandler.csi"));
	TArray64<uint8> FileDataOld;
	TUniquePtr<FLargeMemoryReader> StateReader;
	bool CSIExists = FPaths::FileExists(StateCSIPath);
	if (CSIExists)
	{
		if (FFileHelper::LoadFileToArray(FileDataOld, *StateCSIPath))
		{
			StateReader = MakeUnique<FLargeMemoryReader>(FileDataOld.GetData(), FileDataOld.Num());
		}
	}

	OutState = MakeShared<FParasyteBaseState>();
	if (StateReader.IsValid())
	{
		OutState->GameID = FBinaryReader::ReadUInt64(*StateReader);
		OutState->PoolsAddress = FBinaryReader::ReadUInt64(*StateReader);
		OutState->StringsAddress = FBinaryReader::ReadUInt64(*StateReader);
		const int32 GameDirectoryLength = FBinaryReader::ReadInt32(*StateReader);
		OutState->GameDirectory = FBinaryReader::ReadFString(*StateReader, GameDirectoryLength);

		const uint32 FlagsCount = FBinaryReader::ReadUInt32(*StateReader);
		for (uint32 i = 0; i < FlagsCount; i++)
		{
			const int32 FlagLength = FBinaryReader::ReadInt32(*StateReader);
			FString Flag = FBinaryReader::ReadFString(*StateReader, FlagLength);
			OutState->Flags.Add(Flag);
		}
	}
	return CSIExists;
}
