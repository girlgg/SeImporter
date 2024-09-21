#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "CastAssetFactory.generated.h"

class FLargeMemoryReader;

UCLASS()
class SEIMPORTER_API UCastAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<class UCastImportUI> ImportUI;

	UCastAssetFactory(const FObjectInitializer& ObjectInitializer);

	//~ UObject Interface
	virtual void PostInitProperties() override;
	//~ End of UObject Interface

	//~ Begin UFactor Interface
	virtual bool DoesSupportClass(UClass* Class) override;
	virtual UClass* ResolveSupportedClass() override;
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	                                   const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn,
	                                   bool& bOutOperationCanceled) override;
	//~ End of UFactor Interface

	bool DetectImportType(const FString& InFilename);

protected:
	FText GetImportTaskText(const FText& TaskText) const;

private:
	bool bShowOption;
	bool bDetectImportTypeOnImport;
	bool bOperationCanceled;
};
