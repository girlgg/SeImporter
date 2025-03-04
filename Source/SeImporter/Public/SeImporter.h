// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/CastImportUI.h"

enum class ECastTextureImportType : uint8;

class FSeImporterModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void ImportMapJson();

private:
	void RegisterMenus();

	TSharedRef<SWidget> CreateToolbarDropdown();
	FReply OnConfirmImport(TSharedRef<SWindow> DialogWindow, TSharedPtr<SEditableTextBox> JsonFilePathBox,
	                       TSharedPtr<SEditableTextBox> ModelFolderPathBox, TSharedPtr<SEditableTextBox> ModelPathBox,
	                       TSharedPtr<SEditableTextBox> InGlobalMaterialPathBox);
	FReply OnCancelImport(TSharedRef<SWindow> DialogWindow);
	FReply OnOpenJsonFile(TSharedPtr<SEditableTextBox> JsonFilePathBox);
	FReply OnOpenModelFolder(TSharedPtr<SEditableTextBox> ModelFolderPathBox);

	void HandleImportTypeChanged(ECastTextureImportType NewImportType);
	void OnImportTypeSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> GenerateImportTypeWidget(TSharedPtr<FString> Item);
	FText GetSelectedImportTypeText() const;

	TSharedPtr<SEditableTextBox> GlobalMaterialPathBox;
	ECastTextureImportType CurrentImportType{ECastTextureImportType::CastTIT_Default};
};
