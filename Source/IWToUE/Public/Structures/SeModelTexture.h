#pragma once

#include "Serialization/LargeMemoryReader.h"

class FSeModelTexture
{
public:
	UTexture2D* TextureObject{nullptr};
	FString TextureName{""};
	FString TexturePath{""};
	FString TextureType{""};
	FString TextureSlot{""};

	void ParseTexture(FLargeMemoryReader& Reader);
	static FString NoIllegalSigns(const FString& InString);
};
