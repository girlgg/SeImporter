#pragma once
#include "BaseGame.h"
#include "Structures/MW6GameStructures.h"
#include "Structures/SharedStructures.h"

class FModernWarfare6 : public TBaseGame<FMW6GfxWorld, FMW6GfxWorldTransientZone, FMW6GfxWorldSurfaces, FMW6GfxSurface,
                                         FMW6GfxUgbSurfData, FMW6Material, FMW6GfxWorldStaticModels,
                                         FMW6GfxStaticModelCollection, FMW6GfxStaticModel,
                                         FMW6XModel, FMW6GfxSModelInstanceData, FMW6GfxWorldDrawOffset,
                                         FMW6GfxWorldDrawVerts, FMW6XModelLod,
                                         FMW6XModelSurfs, FMW6XSurfaceShared, FMW6XSurface>
{
public:
	FModernWarfare6(FCordycepProcess* InProcess);

	virtual TArray<FCastTextureInfo> PopulateMaterial(const FMW6Material& Material) override;
	virtual float GetSurfaceScale(const FMW6XSurface& Surface) override;
	virtual FVector3f GetSurfaceOffset(const FMW6XSurface& Surface) override;

	virtual uint64 GetXPakKey(const FMW6XModelSurfs& XModelSurfs, const FMW6XSurfaceShared& Shared) override;

	virtual uint64 GetXyzOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetTangentFrameOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetTexCoordOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetColorOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetIndexDataOffset(const FMW6XSurface& Surface) override;
	virtual uint64 GetPackedIndicesOffset(const FMW6XSurface& Surface) override;
};
