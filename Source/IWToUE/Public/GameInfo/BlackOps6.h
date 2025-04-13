#pragma once
#include "BaseGame.h"
#include "Structures/BO6GameStructures.h"
#include "Structures/SharedStructures.h"

class FBlackOps6 : public TBaseGame<FBO6GfxWorld, FBO6GfxWorldTransientZone, FBO6GfxWorldSurfaces, FBO6GfxSurface,
                                    FBO6GfxUgbSurfData, FBO6Material, FBO6GfxWorldStaticModels,
                                    FBO6GfxStaticModelCollection, FBO6GfxStaticModel, FBO6XModel,
                                    FBO6GfxSModelInstanceData, FBO6GfxWorldDrawOffset, FBO6GfxWorldDrawVerts,
                                    FBO6XModelLod, FBO6XModelSurfs, FBO6XSurfaceShared, FBO6XSurface>
{
public:
	FBlackOps6(FCordycepProcess* InProcess);

	virtual TArray<FCastTextureInfo> PopulateMaterial(const FBO6Material& Material) override;
	virtual FVector3f GetSurfaceOffset(const FBO6XSurface& Surface) override;
	virtual float GetSurfaceScale(const FBO6XSurface& Surface) override;

	virtual uint64 GetXPakKey(const FBO6XModelSurfs& XModelSurfs, const FBO6XSurfaceShared& Shared) override;

	virtual uint64 GetXyzOffset(const FBO6XSurface& Surface) override;
	virtual uint64 GetTangentFrameOffset(const FBO6XSurface& Surface) override;
	virtual uint64 GetTexCoordOffset(const FBO6XSurface& Surface) override;
	virtual uint64 GetColorOffset(const FBO6XSurface& Surface) override;
	virtual uint64 GetIndexDataOffset(const FBO6XSurface& Surface) override;
	virtual uint64 GetPackedIndicesOffset(const FBO6XSurface& Surface) override;
};
