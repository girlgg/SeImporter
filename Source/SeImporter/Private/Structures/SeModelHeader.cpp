#include "Structures/SeModelHeader.h"

#include "Serialization/LargeMemoryReader.h"

SeModelHeader::SeModelHeader(const FString& Filename, FLargeMemoryReader& Reader)
{
	DataPresentFlags = ESeModelDataPresenceFlags::SEMODEL_PRESENCE_MESH;
	GameName = "";
	MeshName =Filename;

	Reader.ByteOrderSerialize(&Data, sizeof(Data));
	uint16_t Version;
	uint16_t HeaderSize;
	Reader << Version;
	Reader << HeaderSize;
	Reader << DataPresentFlags;
	Reader << BonePresentFlags;
	Reader << MeshPresentFlags;
	Reader << BoneCountBuffer;
	Reader << SurfaceCount;
	Reader << MaterialCountBuffer;
	uint8_t ReservedBytes[3];
	Reader.ByteOrderSerialize(&ReservedBytes, sizeof(ReservedBytes));
}
