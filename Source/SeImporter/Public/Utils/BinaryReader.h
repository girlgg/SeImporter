#pragma once

class SEIMPORTER_API FBinaryReader
{
public:
	/*
	 * 读取字符串直到空格
	 */
	static void ReadString(FArchive& Ar, FString* OutText);

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
