#include "Factories/CastAssetFactory.h"

#include "Misc/ScopedSlowTask.h"
#include "EditorReimportHandler.h"
#include "Widgets/CastImportUI.h"
#include "SeLogChannels.h"
#include "Utils/CastImporter.h"

#define LOCTEXT_NAMESPACE "CastFactory"

UCastAssetFactory::UCastAssetFactory(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	Formats.Add(TEXT("cast; models, animations, and more"));
	bCreateNew = false;
	bText = false;
	bEditorImport = true;
	bOperationCanceled = false;
	bShowOption = true;
	bDetectImportTypeOnImport = true;
}

void UCastAssetFactory::PostInitProperties()
{
	Super::PostInitProperties();
	ImportUI = NewObject<UCastImportUI>(this);
	// TODO: Load last dialog state 加载上次对话框设置
}

bool UCastAssetFactory::DoesSupportClass(UClass* Class)
{
	return Class == UStaticMesh::StaticClass() ||
		Class == USkeletalMesh::StaticClass() ||
		Class == UAnimSequence::StaticClass();
}

UClass* UCastAssetFactory::ResolveSupportedClass()
{
	return UStaticMesh::StaticClass();
}

UObject* UCastAssetFactory::HandleExistingAsset(UObject* InParent, FName InName, const FString& InFilename)
{
	UObject* ExistingObject = nullptr;
	if (InParent)
	{
		ExistingObject = StaticFindObject(UObject::StaticClass(), InParent, *InName.ToString());
		if (ExistingObject)
		{
			// TODO: 重新导入
			FReimportHandler* ReimportHandler = nullptr;
			UFactory* ReimportHandlerFactory = nullptr;
			UAssetImportTask* HandlerOriginalImportTask = nullptr;
			bool bIsObjectSupported = false;
			UE_LOG(LogCast, Warning, TEXT("'%s' already exists and cannot be imported"), *InFilename)
		}
	}
	return ExistingObject;
}

void UCastAssetFactory::HandleMaterialImport(FString&& ParentPath, const FString& InFilename,
                                             FCastImporter* CastImporter,
                                             FCastImportOptions* ImportOptions)
{
	FString FilePath = FPaths::GetPath(InFilename);
	FString TextureBasePath;
	FString TextureFormat = ImportOptions->TextureFormat;

	if (ImportOptions->TexturePathType == ECastTextureImportType::CastTIT_Default)
	{
		TextureBasePath = FPaths::Combine(FilePath, TEXT("_images"));
		CastImporter->AnalysisMaterial(ParentPath, FilePath, TextureBasePath, TextureFormat);
	}
	else if (ImportOptions->TexturePathType == ECastTextureImportType::CastTIT_GlobalMaterials)
	{
		TextureBasePath = ImportOptions->GlobalMaterialPath;
		CastImporter->AnalysisMaterial(ParentPath, FilePath, TextureBasePath, TextureFormat);
	}
	else if (ImportOptions->TexturePathType == ECastTextureImportType::CastTIT_GlobalImages)
	{
		TextureBasePath = ImportOptions->GlobalMaterialPath;
		CastImporter->AnalysisMaterial(ParentPath, FilePath, TextureBasePath, TextureFormat, true);
	}
}

UObject* UCastAssetFactory::ExecuteImportProcess(UObject* InParent, FName InName, EObjectFlags Flags,
                                                 const FString& InFilename,
                                                 FCastImporter* CastImporter, FCastImportOptions* ImportOptions,
                                                 FString InCurrentFilename)
{
	UObject* CreatedObject = nullptr;
	if (!CastImporter->ImportFromFile(InCurrentFilename))
	{
		UE_LOG(LogCast, Error, TEXT("Fail to Import Cast File"));
	}
	else
	{
		if (ImportOptions->bImportMaterial && CastImporter->SceneInfo.TotalMaterialNum > 0)
		{
			FString ParentPath = FPaths::GetPath(InParent->GetPathName());
			HandleMaterialImport(MoveTemp(ParentPath), InFilename, CastImporter, ImportOptions);
		}

		if (!ImportOptions->bImportAsSkeletal && ImportOptions->bImportMesh)
		{
			UStaticMesh* NewStaticMesh = CastImporter->ImportStaticMesh(InParent, InName, Flags);
			CreatedObject = NewStaticMesh;
		}
		else if (ImportOptions->bImportMesh && CastImporter->SceneInfo.TotalGeometryNum > 0)
		{
			UPackage* Package = Cast<UPackage>(InParent);

			FName OutputName = *FPaths::GetBaseFilename(InFilename);

			CastScene::FImportSkeletalMeshArgs ImportSkeletalMeshArgs;
			ImportSkeletalMeshArgs.InParent = Package;
			ImportSkeletalMeshArgs.Name = OutputName;
			ImportSkeletalMeshArgs.Flags = Flags;

			USkeletalMesh* BaseSkeletalMesh = CastImporter->ImportSkeletalMesh(ImportSkeletalMeshArgs);
			CreatedObject = BaseSkeletalMesh;
		}
		else if (ImportOptions->bImportAnimations && CastImporter->SceneInfo.bHasAnimation)
		{
			CreatedObject = CastImporter->ImportAnim(InParent, ImportOptions->Skeleton);
		}
	}
	return CreatedObject;
}

UObject* UCastAssetFactory::FactoryCreateFile(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	const FString& InFilename,
	const TCHAR* Parms,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled)
{
	FScopedSlowTask SlowTask(5,
	                         GetImportTaskText(NSLOCTEXT("CastFactory", "BeginReadCastFile", "Opening Cast file.")),
	                         true);
	if (Warn->GetScopeStack().Num() == 0)
	{
		SlowTask.MakeDialog(true);
	}
	SlowTask.EnterProgressFrame(0);

	AdditionalImportedObjects.Empty();
	FString FileExtension = FPaths::GetExtension(InFilename);
	const TCHAR* Type = *FileExtension;

	RETURN_IF_PASS(!IFileManager::Get().FileExists(*InFilename),
	               *FString::Printf(TEXT("File '%s' does not exist"), *InFilename));

	if (bOperationCanceled)
	{
		bOutOperationCanceled = true;

		RETURN_IF_PASS(bOperationCanceled, TEXT("Operator canceled"));
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);

	UObject* CreatedObject = nullptr;

	if (UObject* ExistingObject = HandleExistingAsset(InParent, InName, InFilename))
	{
		return ExistingObject;
	}

	FCastImporter* CastImporter = FCastImporter::GetInstance();
	FSceneCleanupGuard SceneCleanupGuard(CastImporter);

	SlowTask.EnterProgressFrame(1, LOCTEXT("DetectImportType", "Detecting file"));

	if (bDetectImportTypeOnImport)
	{
		RETURN_IF_PASS(!DetectImportType(CurrentFilename), TEXT("Cant detect import type"));
	}

	bool bIsAutomated = IsAutomatedImport();
	bool bShowImportDialog = bShowOption && !bIsAutomated;
	bool bImportAll = false;

	FCastImportOptions* ImportOptions =
		CastImporter->GetImportOptions(ImportUI, bShowImportDialog, bIsAutomated, InParent->GetPathName(),
		                               bOperationCanceled, bImportAll, UFactory::CurrentFilename);
	bOutOperationCanceled = bOperationCanceled;

	SlowTask.EnterProgressFrame(
		3, GetImportTaskText(NSLOCTEXT("CastFactory", "BeginImportingCastTask", "Importing Cast mesh")));

	if (bImportAll)
	{
		bShowOption = false;
	}

	if (!bIsAutomated)
	{
		bDetectImportTypeOnImport = false;
	}

	if (bOperationCanceled)
	{
		AdditionalImportedObjects.Empty();
		CreatedObject = nullptr;
		RETURN_IF_PASS(bOperationCanceled, TEXT("Operator canceled"));
	}

	CreatedObject = ExecuteImportProcess(InParent, InName, Flags, InFilename, CastImporter, ImportOptions,
	                                     CurrentFilename);

	if (!bOperationCanceled && CreatedObject)
	{
		SlowTask.EnterProgressFrame(1, GetImportTaskText(
			                            NSLOCTEXT("CastFactory", "EndingImportingFbxMeshTask",
			                                      "Finalizing mesh import.")));
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, CreatedObject);

	if (CreatedObject)
	{
		//UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		//AssetEditorSubsystem->OpenEditorForAsset(CreatedObject);
	}

	return CreatedObject;
}

bool UCastAssetFactory::DetectImportType(const FString& InFilename)
{
	FCastImporter* CastImporter = FCastImporter::GetInstance();
	int32 ImportType = CastImporter->GetImportType(InFilename);
	if (ImportType == -1)
	{
		CastImporter->ReleaseScene();
		return false;
	}

	return true;
}

FText UCastAssetFactory::GetImportTaskText(const FText& TaskText) const
{
	return bOperationCanceled ? LOCTEXT("CancellingImportingCastTask", "Cancelling Cast import") : TaskText;
}

#undef LOCTEXT_NAMESPACE
