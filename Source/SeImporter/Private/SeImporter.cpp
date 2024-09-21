#include "SeImporter.h"

#include "DesktopPlatformModule.h"
#include "EditorStyleSet.h"
#include "FileHelpers.h"
#include "IDesktopPlatform.h"
#include "ISettingsModule.h"
#include "ActorFactories/ActorFactoryStaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Serialization/LargeMemoryReader.h"
#include "Structures/SeModel.h"
#include "Structures/SeModelMaterial.h"
#include "Structures/SeModelStaticMesh.h"
#include "Structures/SeModelTexture.h"
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
		.Title(FText::FromString(TEXT("导入Json地图")))
		.ClientSize(FVector2D(400, 200))
		.SizingRule(ESizingRule::Autosized);

	// Json文件路径
	TSharedPtr<SEditableTextBox> JsonFilePathBox;
	SAssignNew(JsonFilePathBox, SEditableTextBox)
	.Text(FText::FromString(TEXT("Json文件路径")));
	TSharedPtr<SButton> OpenFileButton;
	SAssignNew(OpenFileButton, SButton)
	.Text(FText::FromString(TEXT("打开Json文件")))
	.OnClicked_Raw(this, &FSeImporterModule::OnOpenJsonFile, JsonFilePathBox);

	// 模型文件夹路径
	TSharedPtr<SEditableTextBox> ModelFolderPathBox;
	SAssignNew(ModelFolderPathBox, SEditableTextBox)
	.Text(FText::FromString(TEXT("模型文件夹路径")));
	// 创建打开模型文件夹对话框按钮
	TSharedPtr<SButton> OpenModelFolderButton;
	SAssignNew(OpenModelFolderButton, SButton)
	.Text(FText::FromString(TEXT("导入模型文件夹")))
	.OnClicked_Raw(this, &FSeImporterModule::OnOpenModelFolder, ModelFolderPathBox);

	// 模型路径
	TSharedPtr<SEditableTextBox> ModelPathBox;
	SAssignNew(ModelPathBox, SEditableTextBox)
	.Text(FText::FromString(TEXT("虚幻模型路径")));

	TSharedPtr<SButton> ConfirmButton, CancelButton;
	SAssignNew(ConfirmButton, SButton)
	.Text(FText::FromString(TEXT("确认")))
	.OnClicked_Raw(this, &FSeImporterModule::OnConfirmImport, DialogWindow, JsonFilePathBox, ModelFolderPathBox,
	               ModelPathBox);
	SAssignNew(CancelButton, SButton)
	.Text(FText::FromString(TEXT("取消")))
	.OnClicked_Raw(this, &FSeImporterModule::OnCancelImport, DialogWindow);


	DialogWindow->SetContent
	(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				JsonFilePathBox.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				OpenFileButton.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				ModelFolderPathBox.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				OpenModelFolderButton.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			ModelPathBox.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
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

FReply FSeImporterModule::OnConfirmImport(TSharedRef<SWindow> DialogWindow,
                                          TSharedPtr<SEditableTextBox> JsonFilePathBox,
                                          TSharedPtr<SEditableTextBox> ModelFolderPathBox,
                                          TSharedPtr<SEditableTextBox> ModelPathBox)
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
				// int32 Index = Object->GetIntegerField(TEXT("Index"));
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
					FString ModelFileBasePath = FPaths::Combine(ModelFolder, Name, Name + "_LOD");
					for (int i = 0; i < 10; ++i)
					{
						FString ModelFilePath = FString::Printf(TEXT("%s%d.semodel"), *ModelFileBasePath, i);
						TArray64<uint8> FileDataOld;
						if (!FFileHelper::LoadFileToArray(FileDataOld, *ModelFilePath))
						{
							continue;
						}
						FString FileName_Fix = FPaths::GetBaseFilename(ModelFilePath);
						FLargeMemoryReader ModelReader(FileDataOld.GetData(), FileDataOld.Num());
						SeModel* Mesh = new SeModel(FileName_Fix, ModelReader);
						ModelReader.Close();

						FString DiskMaterialsPath = FPaths::Combine(ModelFolder, Name);
						FString DiskTexturesPath = FPaths::Combine(DiskMaterialsPath, TEXT("_images"));
						TArray<FSeModelMaterial*> SeModelMaterials;

						FString ModelPackageName = FString::Printf(TEXT("%s/%s"), *GameModelPath, *FileName_Fix);

						for (auto MeshMaterial : Mesh->Materials)
						{
							FSeModelMaterial* CoDMaterial = new FSeModelMaterial();
							CoDMaterial->Header->MaterialName = MeshMaterial.MaterialName;
							FString MaterialContentFileName = FPaths::Combine(
								DiskMaterialsPath, MeshMaterial.MaterialName + "_images.txt");

							if (TArray<FString> MaterialTextContent;
								FFileHelper::LoadFileToStringArray(MaterialTextContent, *MaterialContentFileName))
							{
								for (int32 LineIndex = 1; LineIndex < MaterialTextContent.Num(); ++LineIndex)
								{
									// 导入材质
									if (FSeModelTexture CodTexture;
										SeModelStaticMesh::ImportSeModelTexture(
											CodTexture, GameModelPath,
											MaterialTextContent[LineIndex],
											FPaths::Combine(
												DiskTexturesPath, MeshMaterial.MaterialName),"png"))
									{
										CoDMaterial->Textures.Add(CodTexture);
									}
								}
							}
							SeModelMaterials.Add(CoDMaterial);
						}
						SeModelStaticMesh MeshBuildingClass;
						UUserMeshOptions* Options = NewObject<UUserMeshOptions>();
						MeshBuildingClass.MeshOptions = Options;
						UObject* ParentPackage = CreatePackage(*ModelPackageName);
						ModelMesh = Cast<UStaticMesh>(
							MeshBuildingClass.CreateMesh(ParentPackage, Mesh, SeModelMaterials));
						UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
						break;
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
