#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Structures/SeModelTexture.h"
#include "SeModelAssetFactory.generated.h"

class UUserMeshOptions;

/**
 * 
 */
UCLASS()
class SEIMPORTER_API USeModelAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	USeModelAssetFactory(const FObjectInitializer& ObjectInitializer);

	UPROPERTY()
	UUserMeshOptions* UserSettings;
	bool bImport;
	bool bImportAll;
	bool bCancel;
	//~ UFactory Interface
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	                                   const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn,
	                                   bool& bOutOperationCanceled) override;
};
