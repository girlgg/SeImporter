#include "Factories/SeModelAssetFactory.h"
#include "FileHelpers.h"
#include "Interfaces/IMainFrameModule.h"
#include "Serialization/LargeMemoryReader.h"
#include "Structures/SeModel.h"
#include "Structures/SeModelMaterial.h"
#include "Structures/SeModelStaticMesh.h"
#include "Structures/SeModelTexture.h"
#include "Widgets/SSeModelImportOptions.h"
#include "Widgets/UserMeshOptions.h"

USeModelAssetFactory::USeModelAssetFactory(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	Formats.Add(TEXT("semodel;SeModel File"));
	SupportedClass = UStaticMesh::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
	UserSettings = CreateDefaultSubobject<UUserMeshOptions>(TEXT("Mesh Options"));
}

UObject* USeModelAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
                                                 const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn,
                                                 bool& bOutOperationCanceled)
{
	// Load SEModel File
	TArray64<uint8> FileDataOld;
	if (!FFileHelper::LoadFileToArray(FileDataOld, *Filename))
	{
		return nullptr;
	}

	// Create base Asset
	UObject* MeshCreated = nullptr;

	FString FileName_Fix = FPaths::GetBaseFilename(Filename);
	FLargeMemoryReader Reader(FileDataOld.GetData(), FileDataOld.Num());
	SeModel* Mesh = new SeModel(FileName_Fix, Reader);
	Reader.Close();

	if (!UserSettings->bInitialized)
	{
		TSharedPtr<SSeModelImportOptions> ImportOptionsWindow;
		TSharedPtr<SWindow> ParentWindow;
		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(FText::FromString(TEXT("SEModel Import Options")))
			.SizingRule(ESizingRule::Autosized);
		Window->SetContent
		(
			SAssignNew(ImportOptionsWindow, SSeModelImportOptions)
			.WidgetWindow(Window)
			.MeshHeader(Mesh)
		);
		UserSettings = ImportOptionsWindow.Get()->Options;
		FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
		bImport = ImportOptionsWindow.Get()->ShouldImport();
		bImportAll = ImportOptionsWindow.Get()->ShouldImportAll();
		bCancel = ImportOptionsWindow.Get()->ShouldCancel();
		UserSettings->bInitialized = true;
	}
	if (bCancel)
	{
		bOutOperationCanceled = true;
		return nullptr;
	}

	FString DiskMaterialsPath = FPaths::GetPath(Filename);
	FString DiskTexturesPath = FPaths::Combine(DiskMaterialsPath, TEXT("_images"));
	TArray<FSeModelMaterial*> SeModelMaterials;
	const FString ParentPath = FPaths::GetPath(InParent->GetPathName());

	for (size_t i = 0; i < Mesh->Materials.Num(); i++)
	{
		auto MeshMaterial = Mesh->Materials[i];
		FSeModelMaterial* CoDMaterial = new FSeModelMaterial();
		CoDMaterial->Header->MaterialName = MeshMaterial.MaterialName;
		FString MaterialContentFileName = FPaths::Combine(DiskMaterialsPath, MeshMaterial.MaterialName + "_images.txt");

		if (TArray<FString> MaterialTextContent;
			FFileHelper::LoadFileToStringArray(MaterialTextContent, *MaterialContentFileName))
		{
			for (int32 LineIndex = 1; LineIndex < MaterialTextContent.Num(); ++LineIndex)
			{
				if (FSeModelTexture CodTexture;
					SeModelStaticMesh::ImportSeModelTexture(
						CodTexture, ParentPath, MaterialTextContent[LineIndex],
						FPaths::Combine(
							DiskTexturesPath,
							MeshMaterial.MaterialName),
						UserSettings->MaterialImageFormat))
				{
					CoDMaterial->Textures.Add(CodTexture);
				}
			}
		}
		SeModelMaterials.Add(CoDMaterial);
	}

	SeModelStaticMesh MeshBuildingClass;
	MeshBuildingClass.MeshOptions = UserSettings;
	MeshCreated = MeshBuildingClass.CreateMesh(InParent, Mesh, SeModelMaterials);

	UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

	if (MeshCreated && GEditor)
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OpenEditorForAsset(MeshCreated);
		}
	}

	return MeshCreated;
}
