#pragma once

class FBinaryReader
{
public:
	/*
	 * 读取字符串直到空格
	 */
	static void ReadString(FArchive& Ar, FString* OutText);

	/*
	 * 读取指定字节数
	 */
	static void ReadBytesFromArchive(FArchive& Ar, TArray<uint8>& OutData, int32 BytesToRead);
	static uint64 ReadUInt64(FArchive& Ar);
	static int32 ReadInt32(FArchive& Ar);
	static uint32 ReadUInt32(FArchive& Ar);
	static uint16 ReadUInt16(FArchive& Ar);
	static FString ReadFString(FArchive& Ar, int32 Length);

	template <typename T>
	static TArray<T> ReadList(FArchive& Ar, uint32_t Count);
};

template <typename T>
TArray<T> FBinaryReader::ReadList(FArchive& Ar, uint32_t Count)
{
	TArray<T> Arr;
	T Vec;
	Arr.Reserve(Count);
	for (uint32_t x = 0; x < Count; x++)
	{
		Ar.ByteOrderSerialize(&Vec, sizeof(Vec));;
		Arr.Push(Vec);
	}

	return Arr;
}
