#pragma once
#include "WraithX/CoDAssetType.h"
#include "WraithX/GameProcess.h"

struct FCastModelInfo;
struct FWraithXModel;
struct FWraithXModelLod;
class FGameProcess;

namespace CoDBonesHelper
{
	using BoneIndexVariant = TVariant<TArray<uint8>, TArray<uint16>, TArray<uint32>>;

	void ReadXModelBones(TSharedPtr<FGameProcess> ProcessInstance, FWraithXModel& BaseModel, FWraithXModelLod& ModelLod,
	                     FCastModelInfo& ResultModel);

	void ReadBoneVariantInfo(TSharedPtr<FGameProcess> ProcessInstance, const uint64& ReadAddr, uint64 BoneLen,
						 int32 BoneIndexSize, BoneIndexVariant& OutBoneParentVariant);
};
