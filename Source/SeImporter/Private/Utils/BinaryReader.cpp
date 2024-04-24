#include "Utils/BinaryReader.h"

void FBinaryReader::ReadString(FArchive& Ar, FString* OutText)
{
	char Ch;
	while(Ar << Ch, Ch != 0)
	{
		OutText->AppendChar(Ch);
	}
}
