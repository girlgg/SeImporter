#pragma once

class FSeModelMaterial;
class UMaterialInstanceConstant;

class SeModelMaterialInstance
{
public:
	static UMaterialInterface* CreateMaterialInstance(TArray<FSeModelMaterial*> CoDMaterials,const UObject* ParentPackage,
	                                                     UMaterial* OverrideMasterMaterial);
	static void SetMaterialTextures(FSeModelMaterial* CODMat, FStaticParameterSet& StaticParameters,
	                                UMaterialInstanceConstant*& MaterialAsset, int MaterialIndex);
	static void SetBlendParameters(UMaterialInstanceConstant*& MaterialAsset, int32_t Index);
};
