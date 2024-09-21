#pragma once

#include "SeModelTexture.h"

class FLargeMemoryReader;

enum EBlendingMode
{
    BLEND_VERTEX_SIMPLE, // Multiply Vertex Alpha with Reveal Texture
    BLEND_VERTEX_COMPLEX, // Mixture of Vertex Alpha and Reveal Texture (Game dependant)
    BLEND_MULTIPLY, // Multiply two base colors
    BLEND_TRANSLUCENT
};

class SEIMPORTER_API FSeModelMaterialHeader
{
public:
    FString MaterialName{""};
    EBlendingMode Blending{BLEND_VERTEX_SIMPLE};
    uint8_t SortKey{0};
    uint8_t TextureCount{0};
    uint8_t ConstantCount{0};
};

class SEIMPORTER_API FSeModelMaterial
{
public:
    FSeModelMaterial();

    FSeModelMaterialHeader* Header;
    TArray<FSeModelTexture> Textures;
};
