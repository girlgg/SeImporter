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

	if (!IFileManager::Get().FileExists(*InFilename))
	{
		UE_LOG(LogCast, Error, TEXT("File '%s' not exists"), *InFilename)
		return nullptr;
	}

	ParseParms(Parms);

	CA_ASSUME(InParent);

	if (bOperationCanceled)
	{
		bOutOperationCanceled = true;
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);

	UObject* CreatedObject = nullptr;
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
			return ExistingObject;
		}
	}

	FCastImporter* CastImporter = FCastImporter::GetInstance();

	if (bDetectImportTypeOnImport)
	{
		if (!DetectImportType(CurrentFilename))
		{
			GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, NULL);
			return NULL;
		}
	}

	FCastImportOptions* ImportOptions = CastImporter->GetImportOptions();

	ECastImportType ForcedImportType = Cast_StaticMesh;

	bool bIsAutomated = IsAutomatedImport();
	bool bShowImportDialog = bShowOption && !bIsAutomated;
	bool bImportAll = false;

	ImportOptions =
		CastImporter->GetImportOptions(ImportUI, bShowImportDialog, bIsAutomated, InParent->GetPathName(),
		                               bOperationCanceled, bImportAll, UFactory::CurrentFilename);
	bOutOperationCanceled = bOperationCanceled;
	SlowTask.EnterProgressFrame(
		4, GetImportTaskText(NSLOCTEXT("CastFactory", "BeginImportingCastTask", "Importing Cast mesh")));

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
		return nullptr;
	}

	if (ImportOptions)
	{
		if (!CastImporter->ImportFromFile(CurrentFilename))
		{
			UE_LOG(LogCast, Error, TEXT("Fail to Import Cast File"));
		}
		else
		{
			// 解析材质
			if (ImportOptions->bImportMaterial && CastImporter->SceneInfo.TotalMaterialNum > 0)
			{
				FString FilePath = FPaths::GetPath(InFilename);
				if (ImportOptions->bUseGlobalMaterialsPath)
				{
					CastImporter->AnalysisMaterial(FPaths::GetPath(InParent->GetPathName()),
					                               FilePath,
					                               ImportOptions->GlobalMaterialPath,
					                               ImportOptions->TextureFormat);
				}
				else
				{
					CastImporter->AnalysisMaterial(FPaths::GetPath(InParent->GetPathName()),
					                               FilePath,
					                               FPaths::Combine(FilePath, TEXT("_images")),
					                               ImportOptions->TextureFormat);
				}
			}

			if (!ImportOptions->bImportAsSkeletal && ImportOptions->bImportMesh &&
				CastImporter->SceneInfo.SkinnedMeshNum > 0)
			{
				UStaticMesh* NewStaticMesh = CastImporter->ImportStaticMesh(InParent, InName, Flags);

				if (!bOperationCanceled && NewStaticMesh)
				{
					SlowTask.EnterProgressFrame(1, GetImportTaskText(
						                            NSLOCTEXT("CastFactory", "EndingImportingFbxMeshTask",
						                                      "Finalizing static mesh import.")));
				}

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
	}

	CastImporter->ReleaseScene();
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, CreatedObject);

	if (CreatedObject)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		AssetEditorSubsystem->OpenEditorForAsset(CreatedObject);
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
