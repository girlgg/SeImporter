#include "Structures/SeAnim.h"

#include "Serialization/LargeMemoryReader.h"
#include "Utils/BinaryReader.h"

void FSeAnim::ParseAnim(FLargeMemoryReader& Reader)
{
	ParseHeader(Reader);
	const TArray<FString> BoneNames = ParseBoneNames(Reader);
	const TArray<FAnimationBoneModifier> AnimModifiers = ParseAnimModifiers(Reader);
	ParseBoneData(Reader, AnimModifiers, BoneNames);
}

void FSeAnim::ParseHeader(FLargeMemoryReader& Reader)
{
	Reader.ByteOrderSerialize(&Header.Magic, sizeof(Header.Magic));
	Reader << Header.Version;
	Reader << Header.HeaderSize;
	Reader << Header.AnimType;
	Reader << Header.bLooping;
	Reader << Header.DataFlag;
	Reader << Header.undef_1;
	Reader << Header.undef_2;
	Reader << Header.FrameRate;
	Reader << Header.FrameCountBuffer;
	Reader << Header.BoneCountBuffer;
	Reader << Header.AnimationBoneModifiers;
	Reader.ByteOrderSerialize(&Header.Reserved, sizeof(Header.Reserved));
	Reader << Header.NotificationBuffer;
}

TArray<FString> FSeAnim::ParseBoneNames(FLargeMemoryReader& Reader) const
{
	TArray<FString> BoneNames;
	for (uint32_t an_id = 0; an_id < Header.BoneCountBuffer; an_id++)
	{
		FString bName;
		FBinaryReader::ReadString(Reader, &bName);
		BoneNames.Add(bName);
	}
	return BoneNames;
}

TArray<FAnimationBoneModifier> FSeAnim::ParseAnimModifiers(FLargeMemoryReader& Reader) const
{
	TArray<FAnimationBoneModifier> AnimModifiers;
	for (uint8_t an_mo = 0; an_mo < Header.AnimationBoneModifiers; an_mo++)
	{
		FAnimationBoneModifier AnimModifier;
		int16_t Index;
		if (Header.BoneCountBuffer <= 0xFF)
		{
			// Read it as a byte
			uint8_t ByteIndex;
			Reader << ByteIndex;
			Index = ByteIndex;
		}
		else
		{
			// Read it as a short
			uint16_t ShortIndex;
			Reader << ShortIndex;
			Index = ShortIndex;
		}
		AnimModifier.Index = Index;
		Reader << AnimModifier.AnimType;
		AnimModifiers.Add(AnimModifier);
	}
	return AnimModifiers;
}

void FSeAnim::ParseBoneData(FLargeMemoryReader& Reader, const TArray<FAnimationBoneModifier>& AnimModifiers, const TArray<FString>& BoneNames)
{

	for (uint32_t an_tag = 0; an_tag < Header.BoneCountBuffer; an_tag++)
	{
		TArray<TWraithAnimFrame<FVector3f>> Locations;
		TArray<TWraithAnimFrame<FVector3f>> Scales;
		TArray<TWraithAnimFrame<FQuat4f>> Rotations;
		FBoneInfo BoneInfo;
		BoneInfo.Name = BoneNames[an_tag];
		BoneInfo.Index = an_tag;

		uint8_t random_flag;
		Reader << random_flag;
		if (static_cast<uint8_t>(Header.DataFlag) & static_cast<uint8_t>(ESEAnimDataPresenceFlags::SEANIM_BONE_LOC))
		{
			ParseKeyframeData<FVector3f>(Reader, BoneInfo.BonePositions);
		}
		if (static_cast<uint8_t>(Header.DataFlag) & static_cast<uint8_t>(ESEAnimDataPresenceFlags::SEANIM_BONE_ROT))
		{
			ParseKeyframeData<FQuat4f>(Reader, BoneInfo.BoneRotations);
		}
		if (static_cast<uint8_t>(Header.DataFlag) & static_cast<uint8_t>(ESEAnimDataPresenceFlags::SEANIM_BONE_SCALE))
		{
			ParseKeyframeData<FVector3f>(Reader, BoneInfo.BoneScale);
		}
		BonesInfos.Add(BoneInfo);
	}

}
FQuat4f FSeAnim::FixRotationAbsolute(FQuat4f QuatRot, FQuat4f InitialRot)
{
	if (FMath::Abs(QuatRot.X) <= 0.0005)
	{
		QuatRot.X = InitialRot.X;
	}
	if (FMath::Abs(QuatRot.Y) <= 0.0005)
	{
		QuatRot.Y = InitialRot.Y;
	}
	if (FMath::Abs(QuatRot.Z) <= 0.0005)
	{
		QuatRot.Z = InitialRot.Z;
	}
	if (FMath::Abs(QuatRot.W) <= 0.0005)
	{
		QuatRot.W = InitialRot.W;
	}

	if (FMath::Abs(QuatRot.X) <= 0.0005 && FMath::Abs(QuatRot.Y) <= 0.0005 && FMath::Abs(QuatRot.Z) <= 0.0005 && FMath::Abs(QuatRot.W) <= 0.0005)
	{
		QuatRot = InitialRot;
	}
	return QuatRot;
}
FVector3f FSeAnim::FixPositionAbsolute(FVector3f QuatPos, FVector3f InitialPos)
{
	if (FMath::Abs(QuatPos.X) <= 0.0005)
	{
		QuatPos.X = InitialPos.X;
	}
	if (FMath::Abs(QuatPos.Y) <= 0.0005)
	{
		QuatPos.Y = InitialPos.Y;
	}
	if (FMath::Abs(QuatPos.Z) <= 0.0005)
	{
		QuatPos.Z = InitialPos.Z;
	}

	if (FMath::Abs(QuatPos.X) <= 0.0005 && FMath::Abs(QuatPos.Y) <= 0.0005 && FMath::Abs(QuatPos.Z) <= 0.0005)
	{
		QuatPos = InitialPos;
	}
	return QuatPos;
}
template <typename T>
void FSeAnim::ParseKeyframeData(FLargeMemoryReader& Reader, TArray<TWraithAnimFrame<T>>& KeyframeArray)
{
	uint32_t keys;
	if (Header.FrameCountBuffer <= 0xFF)
	{
		uint8_t byteIndex;
		Reader << byteIndex;
		keys = byteIndex;
	}
	else if (Header.FrameCountBuffer <= 0xFFFF)
	{
		uint16_t byte16;
		Reader << byte16;
		keys = byte16;
	}
	else
	{
		Reader << keys;
	}

	for (uint32_t key = 0; key < keys; key++)
	{
		TWraithAnimFrame<T> AnimFrame;
		uint32_t KeyFrame;
		if (Header.FrameCountBuffer <= 0xFF)
		{
			uint8_t byteIndex;
			Reader << byteIndex;
			KeyFrame = byteIndex;
		}
		else if (Header.FrameCountBuffer <= 0xFFFF)
		{
			uint16_t byte16;
			Reader << byte16;
			KeyFrame = byte16;
		}
		else
		{
			Reader << KeyFrame;
		}
		AnimFrame.Frame = KeyFrame;
		T KeyValue;
		Reader << KeyValue;
		AnimFrame.Value = KeyValue;
		KeyframeArray.Add(AnimFrame);
	}
}
