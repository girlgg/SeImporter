#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#define LOCTEXT_NAMESPACE "CodAssetType"

enum class EWraithAssetType : uint8
{
	// An animation asset
	Animation,
	// An image asset
	Image,
	// A model asset
	Model,
	// A sound file asset
	Sound,
	// A fx file asset
	Effect,
	// A raw file asset
	RawFile,
	// A raw file asset
	Material,
	// A custom asset
	Custom,
	// An unknown asset, not loaded
	Unknown,
};

enum class EWraithAssetStatus : uint8
{
	// The asset is currently loaded
	Loaded,
	// The asset was exported
	Exported,
	// The asset was not loaded
	NotLoaded,
	// The asset is a placeholder
	Placeholder,
	// The asset is being processed
	Processing,
	// The asset had an error
	Error
};

struct FWraithAsset : public TSharedFromThis<FWraithAsset>
{
	// The type of asset we have
	EWraithAssetType AssetType{EWraithAssetType::Unknown};
	// The name of this asset
	FString AssetName{"WraithAsset"};
	// The status of this asset
	EWraithAssetStatus AssetStatus{EWraithAssetStatus::NotLoaded};
	// The size of this asset, -1 = N/A
	int64 AssetSize{-1};
	// Whether or not the asset is streamed
	bool Streamed{false};

	FText GetAssetStatusText() const
	{
		TArray AssetStatusText = {
			LOCTEXT("AssetStatusLoaded", "Loaded"),
			LOCTEXT("AssetStatusExported", "Exported"),
			LOCTEXT("AssetStatusNotLoaded", "NotLoaded"),
			LOCTEXT("AssetStatusPlaceholder", "Placeholder"),
			LOCTEXT("AssetStatusProcessing", "Processing"),
			LOCTEXT("AssetStatusError", "Error")
		};
		return AssetStatusText[static_cast<uint8>(AssetStatus)];
	}

	virtual FText GetAssetTypeText() const
	{
		return LOCTEXT("AssetType", "Unknown");
	}
};

struct FCoDAsset : FWraithAsset
{
	// Whether or not this is a file entry
	bool bIsFileEntry{false};

	// A pointer to the asset in memory
	uint64 AssetPointer{};

	// The assets offset in the loaded pool
	uint32 AssetLoadedIndex{};
};

struct FCoDModel : FCoDAsset
{
	// The bones
	TArray<FString> BoneNames{};
	// The bone count
	uint32 BoneCount{};
	// The cosmetic bone count
	uint32 CosmeticBoneCount{};
	// The lod count
	uint16 LodCount{};

	virtual FText GetAssetTypeText() const
	{
		return LOCTEXT("AssetType", "Model");
	}
};

struct FCoDImage : FCoDAsset
{
	// The width
	uint16 Width{};
	// The height
	uint16 Height{};
	// The format
	uint16 Format{};

	virtual FText GetAssetTypeText() const
	{
		return LOCTEXT("AssetType", "Image");
	}
};

struct FCoDAnim : FCoDAsset
{
	// The bones
	std::vector<std::string> BoneNames;
	// The framerate
	float Framerate;
	// The number of frames
	uint32 FrameCount;
	// The number of frames
	uint32 BoneCount;
	// The number of shapes
	uint32 ShapeCount;

	// Modern games
	bool isSiegeAnim;

	virtual FText GetAssetTypeText() const
	{
		return LOCTEXT("AssetType", "Animation");
	}
};

struct FCoDMaterial : FCoDAsset
{
	int32 ImageCount;

	virtual FText GetAssetTypeText() const
	{
		return LOCTEXT("AssetType", "Material");
	}
};

enum class FCoDSoundDataTypes : uint8
{
	WAV_WithHeader,
	WAV_NeedsHeader,
	FLAC_WithHeader,
	FLAC_NeedsHeader,
	Opus_Interleaved,
	Opus_Interleaved_Streamed,
};

struct FCoDSound : public FCoDAsset
{
	// The full file path of the audio file
	FString FullName;
	// The full file path of the audio file
	FString FullPath;
	// The framerate of this audio file
	uint32 FrameRate;
	// The total frame count
	uint32 FrameCount;
	// Channels count
	uint8 ChannelsCount;
	// Channels count
	uint32 Length;
	// The index of the sound package for this entry
	uint16 PackageIndex;
	// Whether or not this is a localized entry
	bool IsLocalized;

	// The datatype for this entry
	FCoDSoundDataTypes DataType;
	
	virtual FText GetAssetTypeText() const
	{
		return LOCTEXT("AssetType", "Sound");
	}
};

#undef LOCTEXT_NAMESPACE
