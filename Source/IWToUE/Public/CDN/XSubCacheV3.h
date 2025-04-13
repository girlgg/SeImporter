#pragma once

struct FXSubHeaderV2
{
	uint32_t Magic;
	uint16_t Unknown1;
	uint16_t Version;
	uint64_t Unknown;
	uint64_t Type;
	uint64_t Size;
	uint8_t UnknownHashes[1896];
	uint64_t FileCount;
	uint64_t DataOffset;
	uint64_t DataSize;
	uint64_t HashCount;
	uint64_t HashOffset;
	uint64_t HashSize;
	uint64_t Unknown3;
	uint64_t UnknownOffset;
	uint64_t Unknown4;
	uint64_t IndexCount;
	uint64_t IndexOffset;
	uint64_t IndexSize;

	friend FArchive& operator<<(FArchive& Ar, FXSubHeaderV2& Header)
	{
		Ar << Header.Magic;
		Ar << Header.Unknown1;
		Ar << Header.Version;
		Ar << Header.Unknown;
		Ar << Header.Type;
		Ar << Header.Size;
		Ar.Serialize(Header.UnknownHashes, sizeof(Header.UnknownHashes));
		Ar << Header.FileCount;
		Ar << Header.DataOffset;
		Ar << Header.DataSize;
		Ar << Header.HashCount;
		Ar << Header.HashOffset;
		Ar << Header.HashSize;
		Ar << Header.Unknown3;
		Ar << Header.UnknownOffset;
		Ar << Header.Unknown4;
		Ar << Header.IndexCount;
		Ar << Header.IndexOffset;
		Ar << Header.IndexSize;
		return Ar;
	}
};

struct FXSubHashEntryV2
{
	uint64 Key;
	uint64 PackedInfo;
	uint32 PackedInfoEx;

	friend FArchive& operator<<(FArchive& Ar, FXSubHashEntryV2& Entry)
	{
		Ar << Entry.Key;
		Ar << Entry.PackedInfo;
		Ar << Entry.PackedInfoEx;
		return Ar;
	}
};
