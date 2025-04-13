#include "CDN/XSubCacheV2.h"

#include "Compression/OodleDataCompressionUtil.h"
#include "Runtime/OodleDataCompression/Sdks/2.9.11/include/oodle2.h"

bool FXSubCacheV2::DecompressPackageObject(uint64 CacheID, TArray<uint8>& Buffer, int32 DecompressedSize,
                                           TArray<uint8>& OutBuffer)
{
	if (DecompressedSize == 0)
	{
		return true;
	}

	OutBuffer.SetNumUninitialized(DecompressedSize);
	FMemoryReader Reader(Buffer, true);

	while (Reader.Tell() < Reader.TotalSize())
	{
		const int64 BlockPosition = Reader.Tell();

		uint16 Magic;
		Reader << Magic;
		if (Magic != 0xF01D)
		{
			return false;
		}
		uint64 Hash;
		Reader << Hash;
		if (Hash != CacheID)
		{
			return false;
		}

		Reader.Seek(Reader.Tell() + 12);

		uint8 BlockCount;
		Reader << BlockCount;

		if (BlockCount == 0)
		{
			break;
		}

		TArray<FVGXSubBlock> Blocks;
		Blocks.SetNum(BlockCount);
		Reader.Serialize(Blocks.GetData(), BlockCount * sizeof(FVGXSubBlock));

		for (uint32 i = 0; i < BlockCount; i++)
		{
			Reader.Seek(BlockPosition + Blocks[i].BlockOffset);

			TArray<uint8> CompressedData;
			CompressedData.SetNumUninitialized(Blocks[i].CompressedSize);
			Reader.Serialize(CompressedData.GetData(), Blocks[i].CompressedSize);

			switch (Blocks[i].Compression)
			{
			case 0x3:
				{
					FCompression::UncompressMemory(
						NAME_LZ4,
						OutBuffer.GetData() + Blocks[i].DecompressedOffset,
						Blocks[i].DecompressedSize,
						CompressedData.GetData(),
						Blocks[i].CompressedSize
					);
				}
				break;
			case 0x6:
				{
					OodleLZ_Decompress(
						CompressedData.GetData(),
						Blocks[i].CompressedSize,
						OutBuffer.GetData() + Blocks[i].DecompressedOffset,
						Blocks[i].DecompressedSize,
						OodleLZ_FuzzSafe_No,
						OodleLZ_CheckCRC_No,
						OodleLZ_Verbosity_None,
						nullptr,
						0,
						nullptr,
						nullptr,
						nullptr,
						0,
						OodleLZ_Decode_ThreadPhaseAll
					);
				}
				break;
			case 0x0:
				{
					FMemory::Memcpy(
						OutBuffer.GetData() + Blocks[i].DecompressedOffset,
						CompressedData.GetData(),
						Blocks[i].CompressedSize
					);
				}
				break;
			default:
				break;
			}
		}

		Reader.Seek((Reader.Tell() + 0x7F) & ~0x7F);
	}

	return true;
}
