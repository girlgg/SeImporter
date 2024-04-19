#pragma once

class FSeModelSurface;
class SeModelHeader;
class FLargeMemoryReader;

struct FSeModelMeshMaterial
{
	FString MaterialName;
	TArray<FString> TextureNames;
	TArray<FString> TextureTypes;
};

struct FSeModelMeshBone
{
	FString Name;
	uint8 Non;
	uint32_t ParentIndex;
	FVector3f GlobalPosition;
	FRotator3f GlobalRotation;
	FVector3f LocalPosition;
	FRotator3f LocalRotation;
};

class SEIMPORTER_API SeModel
{
public:
	SeModel(const FString& Filename, FLargeMemoryReader& Reader);

	SeModelHeader* Header;
	TArray<FSeModelMeshBone> Bones;
	TArray<FSeModelSurface*> Surfaces;
	TArray<FSeModelMeshMaterial> Materials;
	bool bIsXModel = false;
	bool bIsSkeletal = false;
	int SurfaceVertCounter{0};
	uint8_t MaterialCount{1};
	uint8_t UVSetCount{1};
};
