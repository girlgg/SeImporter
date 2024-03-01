// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "SeAnimAssetFactory.generated.h"

/**
 * 
 */
UCLASS()
class SEIMPORTER_API USeAnimAssetFactory : public UFactory
{
	GENERATED_BODY()

protected:
	USeAnimAssetFactory(const FObjectInitializer& ObjectInitializer);
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	                                   const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn,
	                                   bool& bOutOperationCanceled) override;

	FMeshBoneInfo GetBone(const FString& AnimBoneName);
	int GetBoneIndex(const FString& AnimBoneName);

protected:
	UPROPERTY()
	class USeAnimOptions* SettingsImporter;
	TArray<FMeshBoneInfo> Bones;
	
	bool bImport{false};
	bool bImportAll{false};
};
