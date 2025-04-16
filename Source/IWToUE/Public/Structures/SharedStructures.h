#pragma once
#include "CastManager/CastModel.h"
#include "CastManager/CastScene.h"

struct FCastMeshData
{
	FCastMeshInfo Mesh;
	FCastMaterialInfo Material;
	TArray<FCastTextureInfo> Textures;
};

class FCoDXAnimReader
{
public:
	// The array of bone names.
	TArray<FString> BoneNames;
	// The array of notetracks by frame index.
	TArray<TPair<FString, int32>> Notetracks;
	// The data byte array.
	TArray<uint8> DataBytes;
	// The data byte array.
	TArray<uint8> DataShorts;
	// The data byte array.
	TArray<uint8> DataInts;
	// The data byte array.
	TArray<uint8> RandomDataBytes;
	// The data byte array.
	TArray<uint8> RandomDataShorts;
	// The data byte array.
	TArray<uint8> RandomDataInts;
	// The data byte array.
	TArray<uint8> Indices;
};

struct FQuatData
{
	int16 RotationX;
	int16 RotationY;
	int16 RotationZ;
	int16 RotationW;
};

struct FQuat2Data
{
	int16 RotationZ;
	int16 RotationW;
};
