#pragma once

class FLargeMemoryReader;

namespace HeaderSpace
{
	enum class ESeModelDataPresenceFlags : uint8
	{
		// Whether this model contains a bone block
		SEMODEL_PRESENCE_BONE = 1 << 0,
		// Whether this model contains submesh blocks
		SEMODEL_PRESENCE_MESH = 1 << 1,
		// Whether this model contains inline material blocks
		SEMODEL_PRESENCE_MATERIALS = 1 << 2,

		// The file contains a custom data block
		SEMODEL_PRESENCE_CUSTOM = 1 << 7,
	};

	enum class ESeModelBonePresenceFlags : uint8
	{
		// Whether bones contain global-space matricies
		SEMODEL_PRESENCE_GLOBAL_MATRIX = 1 << 0,
		// Whether bones contain local-space matricies
		SEMODEL_PRESENCE_LOCAL_MATRIX = 1 << 1,

		// Whether bones contain scales
		SEMODEL_PRESENCE_SCALES = 1 << 2,
	};

	enum class ESeModelMeshPresenceFlags : uint8
	{
		// Whether meshes contain at least 1 uv map
		SEMODEL_PRESENCE_UVSET = 1 << 0,

		// Whether meshes contain vertex normals
		SEMODEL_PRESENCE_NORMALS = 1 << 1,

		// Whether meshes contain vertex colors (RGBA)
		SEMODEL_PRESENCE_COLOR = 1 << 2,

		// Whether meshes contain at least 1 weighted skin
		SEMODEL_PRESENCE_WEIGHTS = 1 << 3,
	};

	template<typename EnumName>
	FORCEINLINE uint8 operator&(EnumName LeftEnum, EnumName RightEnum)
	{
		return static_cast<uint8>(LeftEnum) & static_cast<uint8>(RightEnum);
	}
} // HeaderSpace

class SEIMPORTER_API SeModelHeader
{
public:
	explicit SeModelHeader(const FString& Filename, FLargeMemoryReader& Reader);

	char Magic[7]{'S', 'E', 'M', 'o', 'd', 'e', 'l'};
	HeaderSpace::ESeModelDataPresenceFlags DataPresentFlags;
	HeaderSpace::ESeModelBonePresenceFlags BonePresentFlags;
	HeaderSpace::ESeModelMeshPresenceFlags MeshPresentFlags;
	FString GameName{""};
	FString MeshName{""};
	uint32 HeaderBoneCount{0};
	uint32 HeaderMeshCount{0};
	uint32 HeaderMaterialCount{0};
};
