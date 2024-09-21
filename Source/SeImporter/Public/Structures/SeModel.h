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
	FString Name{""};
	uint8 Flags{0};
	uint32 ParentIndex{0};
	FVector3f GlobalPosition{FVector3f::ZeroVector};
	FRotator3f GlobalRotation{FRotator3f::ZeroRotator};
	FVector3f LocalPosition{FVector3f::ZeroVector};
	FRotator3f LocalRotation{FRotator3f::ZeroRotator};
	FVector3f Scale{FVector3f::ZeroVector};
};

class SEIMPORTER_API SeModel
{
public:
	SeModel(const FString& Filename, FLargeMemoryReader& Reader);

	SeModelHeader* Header;
	TArray<FSeModelMeshBone> Bones;
	TArray<FSeModelSurface*> Surfaces;
	TArray<FSeModelMeshMaterial> Materials;
	bool bIsSkeletal = false;
	int SurfaceVertCounter{0};
	uint8_t MaterialCount{1};
	uint8_t UVSetCount{1};
};
