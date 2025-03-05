#include "SeImporter.h"

#include "DesktopPlatformModule.h"
#include "EditorStyleSet.h"
#include "FileHelpers.h"
#include "IDesktopPlatform.h"
#include "ISettingsModule.h"
#include "SeLogChannels.h"
#include "ActorFactories/ActorFactoryStaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Factories/CastAssetFactory.h"
#include "Serialization/LargeMemoryReader.h"
#include "Structures/SeModel.h"
#include "Structures/SeModelMaterial.h"
#include "Structures/SeModelStaticMesh.h"
#include "Structures/SeModelTexture.h"
#include "Utils/CastImporter.h"
#include "Widgets/UserMeshOptions.h"

#define LOCTEXT_NAMESPACE "FSeImporterModule"

void FSeImporterModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSeImporterModule::RegisterMenus));
}

void FSeImporterModule::ShutdownModule()
{
}

void FSeImporterModule::ImportMapJson()
{
	// 创建对话框
	TSharedRef<SWindow> DialogWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Import Json map")))
		.ClientSize(FVector2D(500, 300))
		.SizingRule(ESizingRule::Autosized);

	// Json文件路径
	TSharedPtr<SEditableTextBox> JsonFilePathBox;
	SAssignNew(JsonFilePathBox, SEditableTextBox)
	.Text(FText::FromString(TEXT("Json file path")));

	TSharedPtr<SButton> OpenFileButton;
	SAssignNew(OpenFileButton, SButton)
	.Text(FText::FromString(TEXT("open Json file")))
	.OnClicked_Raw(this, &FSeImporterModule::OnOpenJsonFile, JsonFilePathBox);

	// 模型文件夹路径
	TSharedPtr<SEditableTextBox> ModelFolderPathBox;
	SAssignNew(ModelFolderPathBox, SEditableTextBox)
	.Text(FText::FromString(TEXT("model folder path")));

	// 创建打开模型文件夹对话框按钮
	TSharedPtr<SButton> OpenModelFolderButton;
	SAssignNew(OpenModelFolderButton, SButton)
	.Text(FText::FromString(TEXT("select path")))
	.OnClicked_Raw(this, &FSeImporterModule::OnOpenModelFolder, ModelFolderPathBox);

	// 模型路径
	TSharedPtr<SEditableTextBox> ModelPathBox;
	SAssignNew(ModelPathBox, SEditableTextBox)
	.Text(FText::FromString(TEXT("save to content path")));

	TSharedPtr<SComboBox<TSharedPtr<FString>>> ImportTypeComboBox;

	// 创建下拉菜单选项
	TArray<TSharedPtr<FString>> ImportTypeOptions;
	ImportTypeOptions.Add(MakeShared<FString>(TEXT("Model/Material/Image")));
	ImportTypeOptions.Add(MakeShared<FString>(TEXT("GlobalMaterials/Material/Image")));
	ImportTypeOptions.Add(MakeShared<FString>(TEXT("GlobalImages/Image")));

	// 创建下拉菜单
	SAssignNew(ImportTypeComboBox, SComboBox<TSharedPtr<FString>>)
	.OptionsSource(&ImportTypeOptions)
	.OnSelectionChanged_Raw(this, &FSeImporterModule::OnImportTypeSelectionChanged)
	.OnGenerateWidget_Raw(this, &FSeImporterModule::GenerateImportTypeWidget)
	.Content()
	[
		SNew(STextBlock)
		.Text_Raw(this, &FSeImporterModule::GetSelectedImportTypeText)
	];

	// 全局材质路径编辑框
	SAssignNew(GlobalMaterialPathBox, SEditableTextBox)
	.Text(FText::FromString(TEXT("Global material path")))
	.Visibility(EVisibility::Collapsed);

	TSharedPtr<SButton> ConfirmButton, CancelButton;
	SAssignNew(ConfirmButton, SButton)
	.Text(FText::FromString(TEXT("Confirm")))
	.OnClicked_Raw(this, &FSeImporterModule::OnConfirmImport, DialogWindow, JsonFilePathBox, ModelFolderPathBox,
	               ModelPathBox, GlobalMaterialPathBox);
	SAssignNew(CancelButton, SButton)
	.Text(FText::FromString(TEXT("Cancel")))
	.OnClicked_Raw(this, &FSeImporterModule::OnCancelImport, DialogWindow);


	DialogWindow->SetContent
	(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				JsonFilePathBox.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0, 0, 0)
			[
				OpenFileButton.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				ModelFolderPathBox.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0, 0, 0)
			[
				OpenModelFolderButton.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			ModelPathBox.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Material Path Type")))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				ImportTypeComboBox.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			GlobalMaterialPathBox.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				ConfirmButton.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				CancelButton.ToSharedRef()
			]
		]
	);

	// 展示对话框
	GEditor->EditorAddModalWindow(DialogWindow);
}

void FSeImporterModule::RegisterMenus()
{
	// Register JsonAsAsset toolbar dropdown button
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("SeImporterTool");

	FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitComboButton(
		"SeImporterTool",
		FUIAction(
			FExecuteAction(),
			FCanExecuteAction(),
			FGetActionCheckState()
		),
		FOnGetContent::CreateRaw(this, &FSeImporterModule::CreateToolbarDropdown),
		LOCTEXT("FSeImporterModuleDisplayName", "SeImporterTool"),
		LOCTEXT("FSeImporterModule", "Import Cod20 Map Json"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.ParticleSystem")
	));
}

TSharedRef<SWidget> FSeImporterModule::CreateToolbarDropdown()
{
	FMenuBuilder MenuBuilder(false, nullptr);
	MenuBuilder.BeginSection("JsonAsAssetSection", FText::FromString("Cod20 Map Json Importer"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("FSeImporter", "GitHub"),
			LOCTEXT("FSeImporterTooltip", "Open GitHub Page"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this]()
				{
					FString TheURL = "https://github.com/girlgg/SeImporter";
					FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr);
				})
			),
			NAME_None
		);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("JsonAsAssetButton", "Import Map Json"),
			LOCTEXT("JsonAsAssetButtonTooltip", "Import COD20 Map Json"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FSeImporterModule::ImportMapJson),
				FCanExecuteAction()),
			NAME_None
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

FReply FSeImporterModule::OnOpenJsonFile(TSharedPtr<SEditableTextBox> JsonFilePathBox)
{
	TArray<FString> OutFiles;

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform->OpenFileDialog(nullptr, TEXT("Select JSON file"), FPaths::ProjectContentDir(), TEXT(""),
	                                    TEXT("JSON files (*.json)|*.json"), EFileDialogFlags::None, OutFiles))
	{
		if (OutFiles.Num() > 0)
		{
			// 将用户选择的文件路径显示在输入框中
			JsonFilePathBox->SetText(FText::FromString(OutFiles[0]));
		}
	}

	return FReply::Handled();
}

FReply FSeImporterModule::OnOpenModelFolder(TSharedPtr<SEditableTextBox> ModelFolderPathBox)
{
	FString OutDirectory;

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform->OpenDirectoryDialog(nullptr, TEXT("Select model folder"), FPaths::ProjectDir(), OutDirectory))
	{
		ModelFolderPathBox->SetText(FText::FromString(OutDirectory));
	}

	return FReply::Handled();
}

void FSeImporterModule::HandleImportTypeChanged(ECastTextureImportType NewImportType)
{
	CurrentImportType = NewImportType;

	switch (NewImportType)
	{
	case ECastTextureImportType::CastTIT_Default:
		GlobalMaterialPathBox->SetVisibility(EVisibility::Collapsed);
		break;
	case ECastTextureImportType::CastTIT_GlobalMaterials:
	case ECastTextureImportType::CastTIT_GlobalImages:
		GlobalMaterialPathBox->SetVisibility(EVisibility::Visible);
		break;
	default:
		break;
	}
}

void FSeImporterModule::OnImportTypeSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		if (*NewSelection == TEXT("Model/Material/Image"))
		{
			HandleImportTypeChanged(ECastTextureImportType::CastTIT_Default);
		}
		else if (*NewSelection == TEXT("GlobalMaterials/Material/Image"))
		{
			HandleImportTypeChanged(ECastTextureImportType::CastTIT_GlobalMaterials);
		}
		else if (*NewSelection == TEXT("GlobalImages/Image"))
		{
			HandleImportTypeChanged(ECastTextureImportType::CastTIT_GlobalImages);
		}
	}
}

TSharedRef<SWidget> FSeImporterModule::GenerateImportTypeWidget(TSharedPtr<FString> Item)
{
	return SNew(STextBlock).Text(FText::FromString(*Item));
}

FText FSeImporterModule::GetSelectedImportTypeText() const
{
	switch (CurrentImportType)
	{
	case ECastTextureImportType::CastTIT_Default:
		return FText::FromString(TEXT("Model/Material/Image"));
	case ECastTextureImportType::CastTIT_GlobalMaterials:
		return FText::FromString(TEXT("GlobalMaterials/Material/Image"));
	case ECastTextureImportType::CastTIT_GlobalImages:
		return FText::FromString(TEXT("GlobalImages/Image"));
	default:
		return FText::GetEmpty();
	}
}

FReply FSeImporterModule::OnConfirmImport(TSharedRef<SWindow> DialogWindow,
                                          TSharedPtr<SEditableTextBox> JsonFilePathBox,
                                          TSharedPtr<SEditableTextBox> ModelFolderPathBox,
                                          TSharedPtr<SEditableTextBox> ModelPathBox,
                                          TSharedPtr<SEditableTextBox> InGlobalMaterialPathBox)
{
	FString JsonFilePath = JsonFilePathBox->GetText().ToString();
	// 外部导入的模型文件夹
	FString ModelFolder = ModelFolderPathBox->GetText().ToString();
	FString FileContents;
	FString GameModelPath = ModelPathBox->GetText().ToString();
	if (FFileHelper::LoadFileToString(FileContents, *JsonFilePath))
	{
		TSharedPtr<FJsonValue> JsonParsed;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
		if (FJsonSerializer::Deserialize(Reader, JsonParsed) && JsonParsed.IsValid())
		{
			for (auto& JsonObjectValue : JsonParsed->AsArray())
			{
				FTransform ModelTransform;
				TSharedPtr<FJsonObject> Object = JsonObjectValue->AsObject();
				FString Name = Object->GetStringField(TEXT("Name"));
				float Scale = Object->GetNumberField(TEXT("Scale"));
				ModelTransform.SetScale3D(FVector(Scale, Scale, Scale));

				TSharedPtr<FJsonObject> LocationObject = Object->GetObjectField(TEXT("Location"));
				ModelTransform.SetLocation(FVector(LocationObject->GetNumberField(TEXT("X")) * 2.54,
				                                   LocationObject->GetNumberField(TEXT("Y")) * -2.54,
				                                   LocationObject->GetNumberField(TEXT("Z")) * 2.54));

				TSharedPtr<FJsonObject> RotationObject = Object->GetObjectField(TEXT("Rotation"));
				FRotator LocalRotator = FQuat(RotationObject->GetNumberField(TEXT("X")),
				                              RotationObject->GetNumberField(TEXT("Y")),
				                              RotationObject->GetNumberField(TEXT("Z")),
				                              RotationObject->GetNumberField(TEXT("W"))).Rotator();
				LocalRotator.Yaw *= -1.0f;
				LocalRotator.Roll *= -1.0f;
				ModelTransform.SetRotation(LocalRotator.Quaternion());

				FString ModelPath = FString::Printf(
					TEXT("/Script/Engine.StaticMesh'%s/%s_Static.%s_Static'"), *GameModelPath,
					*Name, *Name);
				UStaticMesh* ModelMesh = Cast<UStaticMesh>(
					StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *ModelPath));
				if (!ModelMesh)
				{
					TArray<FString> FailedModels;
					for (int i = 0; i <= 9; ++i)
					{
						ModelPath = FString::Printf(
							TEXT("/Script/Engine.StaticMesh'%s/%s_LOD%d_Static.%s_LOD%d_Static'"),
							*GameModelPath,
							*Name, i, *Name, i);
						ModelMesh = Cast<
							UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(),NULL, *ModelPath));
						if (ModelMesh)
						{
							break;
						}
					}
				}
				if (!ModelMesh)
				{
					// 目录中没有模型，导入模型
					FString ModelFileBasePath = FPaths::Combine(ModelFolder, Name, Name + "_LOD0");

					FString ModelFilePath = FString::Printf(TEXT("%s.cast"), *ModelFileBasePath);
					FString FileName_Fix = FPaths::GetBaseFilename(ModelFilePath);

					FCastImporter* CastImporter = FCastImporter::GetInstance();
					FSceneCleanupGuard SceneCleanupGuard(CastImporter);

					FCastImportOptions* ImportOptions = CastImporter->GetImportOptions();
					ImportOptions->bImportMaterial = true;
					ImportOptions->GlobalMaterialPath = InGlobalMaterialPathBox->GetText().ToString();
					ImportOptions->TexturePathType = CurrentImportType;
					ImportOptions->TextureFormat = "png";
					ImportOptions->bImportAsSkeletal = false;
					ImportOptions->bImportMesh = true;
					ImportOptions->MaterialType = ECastMaterialType::CastMT_IW9;

					int32 ImportType = CastImporter->GetImportType(ModelFilePath);
					if (ImportType != -1)
					{
						FString ModelPackageName = FPaths::Combine(GameModelPath, FileName_Fix);
						UObject* ParentPackage = CreatePackage(*ModelPackageName);
						EObjectFlags Flags = RF_Public | RF_Standalone;

						ModelMesh = Cast<UStaticMesh>(UCastAssetFactory::ExecuteImportProcess(
							ParentPackage, *FileName_Fix, Flags, ModelFilePath,
							CastImporter, ImportOptions, ModelFilePath));
					}
				}
				if (ModelMesh)
				{
					AActor* ModelActor = GEditor->AddActor(
						GEditor->GetEditorWorldContext().World()->GetCurrentLevel(),
						AStaticMeshActor::StaticClass(), FTransform::Identity);
					AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(ModelActor);
					MeshActor->GetStaticMeshComponent()->SetStaticMesh(ModelMesh);
					MeshActor->SetActorTransform(ModelTransform);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("未找到模型: %s"), *Name);
				}
			}
		}
	}

	DialogWindow->RequestDestroyWindow();
	return FReply::Handled();
}

FReply FSeImporterModule::OnCancelImport(TSharedRef<SWindow> DialogWindow)
{
	DialogWindow->RequestDestroyWindow();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSeImporterModule, SeImporter)
