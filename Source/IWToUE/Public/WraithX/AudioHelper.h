#pragma once

namespace AudioHelper
{
	void WriteWavHeaderToStream(TArray<uint8>& Buffer, uint32 FrameRate, uint32 ChannelsCount, uint32 TotalSize);
	void BuildWavHeader(TArray<uint8>& Buffer, uint32 FrameRate, uint32 ChannelsCount, uint32 TotalSize);
}
