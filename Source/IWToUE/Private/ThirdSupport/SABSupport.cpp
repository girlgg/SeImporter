#include "ThirdSupport/SABSupport.h"
#include "WraithX/AudioHelper.h"

#include "opus.h"

bool SABSupport::DecodeOpusInterLeaved(FWraithXSound& OutSound, TArray<uint8> OpusBuffer, int32 OpusDataOffset,
                                       uint32 FrameRate, uint32 Channels, uint32 FrameCount)
{
	int32 ErrorCode;
	OpusDecoder* Decoder = opus_decoder_create(FrameRate, Channels, &ErrorCode);

	if (ErrorCode != OPUS_OK || Decoder == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Opus decoder creation failed: %hs"), opus_strerror(ErrorCode));
		return false;
	}

	TArray<opus_int16> PCMBuffer;
	PCMBuffer.SetNumUninitialized(960 * Channels);

	int32 OpusConsumed = 0;
	int32 PCMDataSize = 0;
	TArray<uint8> TempWriter;
	TempWriter.SetNumUninitialized(2 * Channels * (FrameCount + 4096));

	const int32 OpusDataSize = OpusBuffer.Num();

	int32 CurrentPosition = 0;

	// 末尾的一帧舍弃，blocksize为0会生成模拟音频，并不是真正的音频数据
	while (OpusConsumed + sizeof(uint16) <= OpusDataSize)
	{
		const int32 ReadOffset = OpusDataOffset + OpusConsumed;

		const uint16 BlockSize = *reinterpret_cast<const uint16*>(&OpusBuffer[ReadOffset]);
		OpusConsumed += sizeof(uint16);

		const uint8* PacketData = OpusBuffer.GetData() + ReadOffset + 2;

		int32 DecoderResult = opus_decode(Decoder,
		                                  PacketData,
		                                  BlockSize,
		                                  PCMBuffer.GetData(),
		                                  960,
		                                  0);
		if (DecoderResult < 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Opus decode failed: %hs"), opus_strerror(DecoderResult));
			return false;
		}

		TempWriter.Append(reinterpret_cast<uint8*>(PCMBuffer.GetData()), 960 * 2 * Channels);

		FMemory::Memcpy(TempWriter.GetData() + CurrentPosition, PCMBuffer.GetData(), 960 * 2 * Channels);
		CurrentPosition += 960 * 2 * Channels;

		OpusConsumed += BlockSize;
		PCMDataSize += 960 * 2 * Channels;
	}
	constexpr uint32 WavHeaderSize = 44;
	OutSound.Data.SetNumUninitialized(PCMDataSize + WavHeaderSize);
	OutSound.DataType = ESoundDataTypes::Wav_WithHeader;
	OutSound.DataSize = PCMDataSize + WavHeaderSize;

	AudioHelper::WriteWavHeaderToStream(OutSound.Data, FrameRate, Channels, PCMDataSize);

	FMemory::Memcpy(OutSound.Data.GetData() + WavHeaderSize, TempWriter.GetData(), PCMDataSize);

	return true;
}
