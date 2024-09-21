#pragma once

struct FCastAnimationInfo;
struct FCastModelInfo;

struct FCastInstanceInfo
{
	FString Name;
	uint64 ReferenceFileHash;
	FVector3f Position;
	FVector4f Rotation;
	FVector3f Scale;
};

struct FCastMetadataInfo
{
	FString Author;
	FString Software;
	FString UpAxis;
};

struct FCastRoot
{
public:
	TArray<FCastModelInfo> Models;
	TArray<FCastAnimationInfo> Animations;
	TArray<FCastInstanceInfo> Instances;
	TArray<FCastMetadataInfo> Metadata;
};
