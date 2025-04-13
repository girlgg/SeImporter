#pragma once

class FXSubCacheV2
{
#pragma pack(push, 1)
	struct FVGXSubBlock
	{
		uint8 Compression;
		uint32 CompressedSize;
		uint32 DecompressedSize;
		uint32 BlockOffset;
		uint32 DecompressedOffset;
		uint32 Unknown;
	};
#pragma pack(pop)

public:
	static bool DecompressPackageObject(uint64 CacheID, TArray<uint8>& Buffer, int32 DecompressedSize,
	                                    TArray<uint8>& OutBuffer);
};
