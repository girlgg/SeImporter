#pragma once
#include "WraithX/CoDAssetType.h"

namespace SABSupport
{
	bool DecodeOpusInterLeaved(FWraithXSound& OutSound, TArray<uint8> OpusBuffer, int32 OpusDataOffset,
	                           uint32 FrameRate, uint32 Channels, uint32 FrameCount);
}
