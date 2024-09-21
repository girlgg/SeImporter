#pragma once

#include "SeAnim.generated.h"

class FLargeMemoryReader;

UENUM()
enum class ESeAnimAnimationType : uint8
{
	SEANIM_ABSOLUTE = 0,
	SEANIM_ADDITIVE = 1,
	SEANIM_RELATIVE = 2,
	SEANIM_DELTA = 3
};

struct FAnimationBoneModifier
{
	uint16_t Index;
	ESeAnimAnimationType AnimType;
};

template <class T>
struct TWraithAnimFrame
{
	// The frame of this animation key
	uint32 Frame;
	// The frame value of this key
	T Value;

	bool HasFrame(const uint32 FrameToCheck) const
	{
		return Frame == FrameToCheck;
	}
};

struct FBoneInfo
{
	FString Name;
	int Index;
	TArray<TWraithAnimFrame<FVector3f>> BonePositions;
	TArray<TWraithAnimFrame<FQuat4f>> BoneRotations;
	TArray<TWraithAnimFrame<FVector3f>> BoneScale;

	FVector3f GetPositionAtFrame(const uint32 FrameAsked) const
	{
		for (const auto& Var : BonePositions)
		{
			if (Var.HasFrame(FrameAsked))
			{
				return Var.Value;
			}
		}
		return FVector3f(-1, -1, -1);
	}

	FQuat4f GetRotationAtFrame(const uint32_t FrameAsked) const
	{
		for (const auto& Var : BoneRotations)
		{
			if (Var.HasFrame(FrameAsked))
			{
				return Var.Value;
			}
		}
		return FQuat4f(-1, -1, -1, -1);
	}

	FVector3f GetScaleAtFrame(const uint32 FrameAsked) const
	{
		for (const auto& Var : BoneScale)
		{
			if (Var.HasFrame(FrameAsked))
			{
				return Var.Value;
			}
		}
		return FVector3f(-1, -1, -1);
	}
};


struct FAnimHeader
{
	char Magic[6];
	uint16_t Version;
	uint16_t HeaderSize;
	ESeAnimAnimationType AnimType;
	uint8_t bLooping;
	uint8_t DataFlag;
	uint8_t undef_1;
	uint16_t undef_2;
	float FrameRate;
	uint32_t FrameCountBuffer;
	uint32_t BoneCountBuffer;
	uint8_t AnimationBoneModifiers;
	uint8_t Reserved[3];
	uint32_t NotificationBuffer;
};

enum class ESEAnimDataPresenceFlags : uint8
{
	SEANIM_BONE_LOC = 1 << 0,
	SEANIM_BONE_ROT = 1 << 1,
	SEANIM_BONE_SCALE = 1 << 2,
	SEANIM_PRESENCE_BONE = SEANIM_BONE_LOC | SEANIM_BONE_ROT | SEANIM_BONE_SCALE,
	SEANIM_PRESENCE_NOTE = 1 << 6,
	SEANIM_PRESENCE_CUSTOM = 1 << 7
};

struct FSeAnim
{
	void ParseAnim(FLargeMemoryReader& Reader);
	void ParseHeader(FLargeMemoryReader& Reader);
	TArray<FString> ParseBoneNames(FLargeMemoryReader& Reader) const;
	TArray<FAnimationBoneModifier> ParseAnimModifiers(FLargeMemoryReader& Reader) const;
	void ParseBoneData(FLargeMemoryReader& Reader, const TArray<FAnimationBoneModifier>& AnimModifiers,
	                   const TArray<FString>& BoneNames);
	FAnimHeader Header;
	TArray<FBoneInfo> BonesInfos;
	template <typename T>
	void ParseKeyframeData(FLargeMemoryReader& Reader, TArray<TWraithAnimFrame<T>>& KeyframeArray);
	static FQuat4f FixRotationAbsolute(FQuat4f QuatRot, FQuat4f InitialRot);
	static FVector3f FixPositionAbsolute(FVector3f QuatPos, FVector3f InitialPos);
};
