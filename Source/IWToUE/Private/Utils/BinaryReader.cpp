#include "Utils/BinaryReader.h"

void FBinaryReader::ReadString(FArchive& Ar, FString* OutText)
{
	char Ch;
	while (Ar << Ch, Ch != 0)
	{
		OutText->AppendChar(Ch);
	}
}

void FBinaryReader::ReadBytesFromArchive(FArchive& Ar, TArray<uint8>& OutData, int32 BytesToRead)
{
	OutData.SetNumUninitialized(BytesToRead);

	Ar.Serialize(OutData.GetData(), BytesToRead);
}

uint64 FBinaryReader::ReadUInt64(FArchive& Ar)
{
	uint64 Value;
	Ar.Serialize(&Value, sizeof(uint64));
	return Value;
}

int32 FBinaryReader::ReadInt32(FArchive& Ar)
{
	int32 Value;
	Ar.Serialize(&Value, sizeof(int32));
	return Value;
}

uint32 FBinaryReader::ReadUInt32(FArchive& Ar)
{
	uint32 Value;
	Ar.Serialize(&Value, sizeof(uint32));
	return Value;
}

uint16 FBinaryReader::ReadUInt16(FArchive& Ar)
{
	uint16 Value;
	Ar.Serialize(&Value, sizeof(uint16));
	return Value;
}

FString FBinaryReader::ReadFString(FArchive& Ar, int32 Length)
{
	TArray<uint8> Data;
	ReadBytesFromArchive(Ar, Data, Length);

	if (Data.IsEmpty())
	{
		return FString();
	}
	Data.Add(0);

	return FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(Data.GetData())));
}
