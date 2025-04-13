#pragma once

struct FBO6GfxWorldSurfaces
{
	uint32 Count;												// 0
	uint8 Pad_04[4];											// 4
	uint32 UgbSurfDataCount;									// 8
	uint32 MaterialCount;										// 12
	uint32 MaterialFlagsCount;									// 16
	uint8 Pad_10[120 - 20];										// 20
	uint64 Surfaces;											// 120
	uint8 Pad_124[136 - 128];									// 128
	uint64 SurfaceBounds;										// 136
	uint8 Pad_78[160 - 144];									// 144
	uint64 Materials;											// 160
	uint8 Pad_168[176 - 168];									// 168
	uint64 SurfaceMaterialFlags;								// 176
	uint8 Pad_184[192 - 184];									// 184
	uint64 UgbSurfData;											// 192
	uint64 WorldDrawOffsets;									// 200
	uint8 Pad_B8[320 - 208];									// 208
	uint32 BtndSurfacesCount;									// 320
	uint64 BtndSurfaces;										// 328
	uint8 Pad_334[352 - 336];									// 336
	uint64 unkPtr0;												// 352
	uint8 Pad_138[392 - 360];									// 360 Padding to 392 bytes
};

struct FBO6GfxWorldStaticModels
{
	uint8 Pad_00[4];											// 0
	uint32 SModelCount;											// 4
	uint32 CollectionsCount;									// 8
	uint32 InstanceCount;										// 12
	uint8 Pad_10[80 - 16];										// 16
	uint64 Surfaces;											// 80
	uint8 Pad_88[104 - 88];										// 88
	uint64 SModels;												// 104
	uint8 Pad_112[120 - 112];									// 96
	uint64 Collections;											// 120
	uint8 Pad_128[200 - 128];									// 128
	uint64 InstanceData;										// 200
};

struct FBO6GfxWorld
{
	uint64 Hash;												// 0
	uint64 BaseName;											// 8
	uint8 Pad_10[216 - 16];										// 16
	FBO6GfxWorldSurfaces Surfaces;								// 216
	FBO6GfxWorldStaticModels SModels;							// 608
	uint8 Pad_816[27548 - 816];									// 816
	uint32 TransientZoneCount;									// 27548
	uint64 TransientZones[1536];								// 27552
};

struct FBO6GfxWorldDrawVerts
{
	uint32 PosSize;												// 0
	uint32 IndexCount;											// 4
	uint32 TableCount;											// 8
	uint8 Pad_12[16 - 12];										// 12
	uint32 PackedIndicesSize;									// 16
	uint64 PosData;												// 24
	uint64 Indices;												// 32
	uint64 TableData;											// 40
	uint64 PackedIndices;										// 48
};

struct FBO6GfxWorldTransientZone
{
	uint64 Hash;												// 0
	uint64 UnkPtr0;												// 8
	uint8 Pad_16[24 - 16];										// 16
	FBO6GfxWorldDrawVerts DrawVerts;							// 24
	uint8 Pad_4C[584 - 80];										// 80 Padding to 584 bytes
};

struct FBO6GfxSurface
{
	uint32 PosOffset;											// 0
	uint32 BaseIndex;											// 4
	uint32 TableIndex;											// 8
	uint32 UgbSurfDataIndex;									// 12
	uint32 MaterialIndex;										// 16
	uint16 TriCount;											// 20
	uint16 PackedIndicesTableCount;								// 22
	uint16 VertexCount;											// 24
	uint16 Unk3;												// 26
	uint32 Unk4;												// 28
	uint32 PackedIndicesOffset;									// 32
};

struct FBO6GfxWorldDrawOffset
{
	float X;													// 0
	float Y;													// 4
	float Z;													// 8
	float Scale;												// 12
};

struct FBO6GfxUgbSurfData
{
	FBO6GfxWorldDrawOffset WorldDrawOffset;						// 0
	uint32 TransientZoneIndex;									// 16
	uint32 Unk0;												// 20
	uint32 Unk1;												// 24
	uint32 LayerCount;											// 28
	uint32 VertexCount;											// 32
	uint32 Unk2;												// 36
	uint32 XyzOffset;											// 40
	uint32 TangentFrameOffset;									// 44
	uint32 LMapOffset;											// 48
	uint32 ColorOffset;											// 52
	uint32 TexCoordOffset;										// 56
	uint32 UnkDataOffset;										// 60
	uint32 AccumulatedUnk2;										// 64
	uint32 NormalTransformOffset[7];							// 68
	uint32 DisplacementOffset[8];								// 96
};

struct FBO6GfxImage
{
	uint64 Hash;
};

struct FBO6Material
{
	uint64 Hash;												// 0
	uint8 Pad_08[24 - 8];										// 8
	uint8 TextureCount;											// 24
	uint8 Pad_19;												// 25
	uint8 LayerCount;											// 26
	uint8 ImageCount;											// 27
	uint8 Pad_1C[40 - 28];										// 28
	uint64 TextureTable;										// 40
	uint64 ImageTable;											// 48
	uint8 Pad_38[128 - 56];										// 56 Padding to 128 bytes
};

struct FBO6MaterialTextureDef 
{
	uint8 Index;
	uint8 ImageIdx;
};

struct FBO6GfxStaticModelCollection
{
	uint32 FirstInstance;										// 0
	uint32 InstanceCount;										// 4
	uint16 SModelIndex;											// 8
	uint16 TransientGfxWorldPlaced;								// 10
	uint16 ClutterIndex;										// 12
	uint8 Flags;												// 14
	uint8 Pad;													// 15
};

struct FBO6GfxStaticModel
{
	uint64 XModel;												// 0
	uint8 Flags;												// 8
	uint8 FirstMtlSkinIndex;									// 9
	uint16 FirstStaticModelSurfaceIndex;						// 10
};

struct FBO6XModel
{
	uint64 Hash;												// 0
	uint64 NamePtr;												// 8
	uint16 NumSurfs;											// 16
	uint8 NumLods;												// 18
	uint8 Pad_20[40 - 19];										// 19
	float Scale;												// 40
	uint8 Pad_44[104 - 44];										// 44
	uint64 XModelPackedDataPtr;									// 104
	uint8 Pad_112[144 - 112];									// 112
	uint64 MaterialHandlesPtr;									// 144
	uint64 LodInfo;												// 152
	uint8 Pad_160[248 - 160];									// 160 Padding to 248 bytes
};

struct FBO6GfxSModelInstanceData
{
	int32 Translation[3];										// 0
	uint16 Orientation[4];										// 12
	uint16 HalfFloatScale;										// 20
};

struct FBO6XModelLod
{
	uint64 MeshPtr;
	uint64 SurfsPtr;

	float LodDistance[3];

	uint16 NumSurfs;
	uint16 SurfacesIndex;

	uint8 Padding[40];
};

struct FBO6XModelSurfs
{
	uint64 Hash;												// 0
	uint64 Surfs;												// 8
	uint64 Shared;												// 16
	uint16 NumSurfs;											// 24
	uint8 Pad_32[64 - 32];										// 32 Padding to 64 bytes
};

struct FBO6XSurfaceShared
{
	int64 Data;													// 0
	uint64 XPakKey;												// 8
	uint32 DataSize;											// 16
	int32 Flags;												// 20
};

struct FBO6Bounds
{
	FVector3f MidPoint;											// 0
	FVector3f HalfSize;											// 12
};

struct FBO6XSurface
{
	uint8 Pad_00[20];											// 0
    uint32 VertCount;											// 20
    uint32 TriCount;											// 24
	uint32 PackedIndicesTableCount;								// 28
    uint8 Pad_08[60-32];										// 32
    float OverrideScale;										// 60
    // uint32 Offsets[14];										// 64
    uint32 SharedVertDataOffset;								// 64
    uint32 SharedUVDataOffset;									// 68
    uint32 SharedTangentFrameDataOffset;						// 72
    uint32 SharedIndexDataOffset;								// 76
    uint32 PackedIndiciesTableOffset;							// 80
    uint32 SharedPackedIndicesOffset;							// 84
    uint32 SharedColorDataOffset;								// 88
    uint32 SharedSecondUVDataOffset;							// 92
    uint32 SecondUVOffset;										// 96
    uint8 Pad_100[120 - 100];									// 100
    uint64 Shared;												// 120
    uint8 Pad_124[176 - 128];									// 128
    FBO6Bounds SurfBounds;										// 176
    uint8 Pad_200[224 - 200];									// 200
};