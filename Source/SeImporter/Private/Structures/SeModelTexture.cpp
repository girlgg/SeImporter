#include "Structures/SeModelTexture.h"

#include "Utils/BinaryReader.h"

void FSeModelTexture::ParseTexture(FLargeMemoryReader& Reader)
{
	FBinaryReader::ReadString(Reader, &TextureName);
	TextureName = TextureName.Replace(TEXT("~"),TEXT("_")).Replace(TEXT("$"),TEXT("_"));
	FBinaryReader::ReadString(Reader, &TextureType);
}

FString FSeModelTexture::NoIllegalSigns(const FString& InString)
{
	return InString.Replace(TEXT("~"),TEXT("_")).Replace(TEXT("$"),TEXT("_")).Replace(TEXT("&"),TEXT("_")).Replace(
		TEXT("#"),TEXT("_"));
}
