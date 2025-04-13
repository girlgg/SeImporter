#include "Structures/SeModelHeader.h"

#include "Serialization/LargeMemoryReader.h"

SeModelHeader::SeModelHeader(const FString& Filename, FLargeMemoryReader& Reader)
{
	MeshName = Filename;

	Reader.ByteOrderSerialize(&Magic, sizeof(Magic));
	uint16 Version;
	uint16 HeaderSize;
	Reader << Version;
	Reader << HeaderSize;

	Reader << DataPresentFlags;
	Reader << BonePresentFlags;
	Reader << MeshPresentFlags;

	Reader << HeaderBoneCount;
	Reader << HeaderMeshCount;
	Reader << HeaderMaterialCount;

	char TmpData[20];
	Reader.ByteOrderSerialize(&TmpData, HeaderSize - 17);
}
