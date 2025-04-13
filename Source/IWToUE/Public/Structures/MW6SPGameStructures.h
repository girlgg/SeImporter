#pragma once

struct FMW6SPGfxWorld
{
	uint64 Hash;												// 0
	uint64 BaseName;											// 8
	uint8 Pad_16[192 - 16];										// 16
	FMW6GfxWorldSurfaces Surfaces;								// 192
	uint8 Pad_504[560 - 504];									// 504
	FMW6GfxWorldStaticModels SModels;							// 560
	uint8 Pad_768[5652 - 944];									// 944
	uint32 TransientZoneCount;									// 5652
	uint64 TransientZones[1536];								// 5656
};

struct FMW6SPMaterial
{
	uint64 Hash;												// 0
	uint8 Pad_08[24 - 8];										// 8
	uint8 TextureCount;											// 24
	uint8 Pad_25[27 - 25];										// 25
	uint8 LayerCount;											// 27
	uint8 Pad_28[48 - 28];										// 28
	uint64 TextureTable;										// 48
	uint8_t Padding3[112 - 56];									// 56
};

struct FMW6SPMaterialTextureDef
{
	uint8 Index;
	uint8 Padding[7];
	uint64 ImagePtr;
};

struct FMW6SPXModel
{
    uint64 Hash;
    uint64 NamePtr;
    uint16 NumSurfaces;
    uint8 NumLods;
    uint8 MaxLods;

    uint8 Padding[13];

    uint8 NumBones;
    uint16 NumRootBones;
    uint16 UnkBoneCount;

    uint8 Padding3[0xA2];

    uint64 BoneIDsPtr;
    uint64 ParentListPtr;
    uint64 RotationsPtr;
    uint64 TranslationsPtr;
    uint64 PartClassificationPtr;
    uint64 BaseMatriciesPtr;
    uint64 UnknownPtr;
    uint64 UnknownPtr2;
    uint64 MaterialHandlesPtr;
    uint64 LodInfo;

    uint8 Padding4[120];
};
/*struct FMW6SPXModel
{
	uint64 Hash;												// 0
	uint64 Name;												// 8
	uint16 NumSurfs;											// 16
	uint8 Pad_18[40 - 18];										// 18
	float Scale;												// 40
	uint8 Pad_44[264 - 44];										// 44
	uint64 MaterialHandles;										// 264
	uint64 LodInfo;												// 272
};*/
