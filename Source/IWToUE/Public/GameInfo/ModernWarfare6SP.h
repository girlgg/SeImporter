#pragma once
#include "BaseGame.h"
#include "Structures/MW6GameStructures.h"
#include "Structures/MW6SPGameStructures.h"
#include "Structures/SharedStructures.h"

class FModernWarfare6SP : public TBaseGame<FMW6SPGfxWorld, FMW6GfxWorldTransientZone, FMW6GfxWorldSurfaces,
                                           FMW6GfxSurface, FMW6GfxUgbSurfData, FMW6SPMaterial, FMW6GfxWorldStaticModels,
                                           FMW6GfxStaticModelCollection, FMW6GfxStaticModel, FMW6SPXModel,
                                           FMW6GfxSModelInstanceData, FMW6GfxWorldDrawOffset, FMW6GfxWorldDrawVerts,
                                           FMW6XModelLod, FMW6XModelSurfs, FMW6XSurfaceShared, FMW6XSurface>
{
public:
	FModernWarfare6SP(FCordycepProcess* InProcess);

	virtual TArray<FCastTextureInfo> PopulateMaterial(const FMW6SPMaterial& Material) override;
	virtual FVector3f GetSurfaceOffset(const FMW6XSurface& Surface) override;
	virtual float GetSurfaceScale(const FMW6XSurface& Surface) override;

	virtual uint64 GetXPakKey(const FMW6XModelSurfs& XModelSurfs, const FMW6XSurfaceShared& Shared) override;

	virtual uint64 GetXyzOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetTangentFrameOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetTexCoordOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetColorOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetIndexDataOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetPackedIndicesOffset(const FMW6XSurface& Surface) override;
};
