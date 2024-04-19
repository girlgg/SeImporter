#include "Structures/SeModelMaterialInstance.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Structures/SeModelMaterial.h"
#include "Structures/SeModelTexture.h"
#include "UObject/SavePackage.h"

UMaterialInterface* SeModelMaterialInstance::CreateMaterialInstance(TArray<FSeModelMaterial*> CoDMaterials,
                                                                    const UObject* ParentPackage,
                                                                    UMaterial* OverrideMasterMaterial)
{
	UMaterialInterface* UnrealMaterialFinal = nullptr;
	// Set full package path
	FString FullMaterialName = CoDMaterials[0]->Header->MaterialName;
	FString MaterialType = TEXT("Normal");
	for (const auto Mat : CoDMaterials)
	{
		for (const auto& Texture : Mat->Textures)
		{
			if (Texture.TextureType == TEXT("Alpha"))
			{
				MaterialType = TEXT("Alpha");
			}
			else if (Texture.TextureType == TEXT("Mask") && MaterialType == TEXT("Normal"))
			{
				MaterialType = TEXT("Mask");
			}
		}
	}
	for (int i = 1; i < CoDMaterials.Num(); i++)
	{
		FullMaterialName += "__" + CoDMaterials[i]->Header->MaterialName;
	}

	const auto MaterialInstanceFactory = NewObject<UMaterialInstanceConstantFactoryNew>();
	// Master Material
	FString MaterialPath{""};
	if (MaterialType == TEXT("Normal"))
	{
		MaterialPath = "/SeImporter/BaseMaterials/BaseMat.BaseMat";
	}
	else if (MaterialType == TEXT("Alpha"))
	{
		MaterialPath = "/SeImporter/BaseMaterials/BaseMatWithAlpha.BaseMatWithAlpha";
	}
	else if (MaterialType == TEXT("Mask"))
	{
		MaterialPath = "/SeImporter/BaseMaterials/BaseMatWithAlphaMask.BaseMatWithAlphaMask";
	}

	MaterialInstanceFactory->InitialParent = !OverrideMasterMaterial
		                                         ? Cast<UMaterial>(
			                                         StaticLoadObject(UMaterial::StaticClass(), nullptr, *MaterialPath))
		                                         : OverrideMasterMaterial;
	const auto MaterialPackage = CreatePackage(
		*FPaths::Combine(FPaths::GetPath(ParentPackage->GetPathName()),TEXT("Materials/"), FullMaterialName));
	check(MaterialPackage);

	MaterialPackage->FullyLoad();
	MaterialPackage->Modify();
	UObject* MaterialInstObject = MaterialInstanceFactory->FactoryCreateNew(
		UMaterialInstanceConstant::StaticClass(), MaterialPackage, *FullMaterialName, RF_Standalone | RF_Public, NULL,
		GWarn);
	UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(MaterialInstObject);
	// Notify the asset registry
	FAssetRegistryModule::AssetCreated(MaterialInstance);
	// Set the dirty flag so this package will get saved later

	// Get static parameters
	FStaticParameterSet StaticParameters;
	MaterialInstance->GetStaticParameterValues(StaticParameters);

	const auto CODMat = CoDMaterials[0];
	SetMaterialTextures(CODMat, StaticParameters, MaterialInstance, -1);
	MaterialInstance->UpdateStaticPermutation(StaticParameters);

	UnrealMaterialFinal = MaterialInstance;
	UnrealMaterialFinal->PreEditChange(nullptr);
	UnrealMaterialFinal->PostEditChange();
	UnrealMaterialFinal->GetPackage()->FullyLoad();
	UnrealMaterialFinal->MarkPackageDirty();

	return UnrealMaterialFinal;
}

void SeModelMaterialInstance::SetMaterialTextures(FSeModelMaterial* CODMat, FStaticParameterSet& StaticParameters,
                                                  UMaterialInstanceConstant*& MaterialAsset, int MaterialIndex)
{
	if (!MaterialAsset || !MaterialAsset->IsValidLowLevel())
	{
		return;
	}
	for (auto Texture : CODMat->Textures)
	{
		if (!Texture.TextureObject) { continue; }
		UTexture* TextureAsset = Cast<UTexture>(Texture.TextureObject);
		FString& TextureType = Texture.TextureType;
		if (TextureAsset && !TextureType.IsEmpty())
		{
			for (auto& SwitchParam : StaticParameters.StaticSwitchParameters)
			{
				SwitchParam.bOverride = true;
				SwitchParam.Value = true;
			}
			TextureType = Texture.TextureSlot;
			FMaterialParameterInfo TextureParameterInfo(*TextureType, GlobalParameter, MaterialIndex);
			MaterialAsset->SetTextureParameterValueEditorOnly(TextureParameterInfo, TextureAsset);
		}
	}
}

void SeModelMaterialInstance::SetBlendParameters(UMaterialInstanceConstant*& MaterialAsset, int32_t Index)
{
}
