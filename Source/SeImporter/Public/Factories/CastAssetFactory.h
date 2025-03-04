#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Utils/CastImporter.h"
#include "CastAssetFactory.generated.h"

class FLargeMemoryReader;

#define RETURN_IF_PASS(condition, message) \
if (condition) { \
UE_LOG(LogCast, Error, TEXT("%s"), message); \
GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr); \
return nullptr; \
}

class FSceneCleanupGuard
{
public:
	FSceneCleanupGuard(FCastImporter* Importer) : ImporterPtr(Importer)
	{
	}

	~FSceneCleanupGuard() { if (ImporterPtr) ImporterPtr->ReleaseScene(); }

private:
	FCastImporter* ImporterPtr;
};

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
	UObject* HandleExistingAsset(UObject* InParent, FName InName, const FString& InFilename);
	static void HandleMaterialImport(FString&& ParentPath, const FString& InFilename, FCastImporter* CastImporter,
	                                 FCastImportOptions* ImportOptions);
	static UObject* ExecuteImportProcess(UObject* InParent, FName InName, EObjectFlags Flags, const FString& InFilename,
	                                     FCastImporter* CastImporter, FCastImportOptions* ImportOptions,
	                                     FString InCurrentFilename);
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
