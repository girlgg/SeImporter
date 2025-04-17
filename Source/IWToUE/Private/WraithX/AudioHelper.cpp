#include "WraithX/AudioHelper.h"

void AudioHelper::WriteWavHeaderToStream(TArray<uint8>& Buffer, uint32 FrameRate, uint32 ChannelsCount,
                                         uint32 TotalSize)
{
	BuildWavHeader(Buffer, FrameRate, ChannelsCount, TotalSize);
}

void AudioHelper::BuildWavHeader(TArray<uint8>& Buffer, uint32 FrameRate, uint32 ChannelsCount, uint32 TotalSize)
{
	int32 FileSize = 36 + TotalSize;

	FMemory::Memcpy(Buffer.GetData() + 0, "RIFF", 4);
	FMemory::Memcpy(Buffer.GetData() + 4, &FileSize, 4);
	FMemory::Memcpy(Buffer.GetData() + 8, "WAVEfmt ", 8);

	uint32 FormatData = 16;			// PCM 格式数据块大小
	uint16 Encoding = 1;				// PCM编码
	uint16 BitsPerSample = 16;			// 16位采样
	uint16 BlockAlign = ChannelsCount * (BitsPerSample / 8);
	uint32 AverageBps = FrameRate * BlockAlign;

	FMemory::Memcpy(Buffer.GetData() + 16, &FormatData, 4);
	FMemory::Memcpy(Buffer.GetData() + 20, &Encoding, 2);
	FMemory::Memcpy(Buffer.GetData() + 22, &ChannelsCount, 2);
	FMemory::Memcpy(Buffer.GetData() + 24, &FrameRate, 4);
	FMemory::Memcpy(Buffer.GetData() + 28, &AverageBps, 4);
	FMemory::Memcpy(Buffer.GetData() + 32, &BlockAlign, 2);
	FMemory::Memcpy(Buffer.GetData() + 34, &BitsPerSample, 2);

	FMemory::Memcpy(Buffer.GetData() + 36, "data", 4);
	FMemory::Memcpy(Buffer.GetData() + 40, &TotalSize, 4);
}
