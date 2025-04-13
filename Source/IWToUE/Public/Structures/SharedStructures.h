#pragma once
#include "CastManager/CastModel.h"
#include "CastManager/CastScene.h"

struct FCastMeshData
{
	FCastMeshInfo Mesh;
	FCastMaterialInfo Material;
	TArray<FCastTextureInfo> Textures;
};
