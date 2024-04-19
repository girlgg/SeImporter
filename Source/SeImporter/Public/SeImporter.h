// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

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
	                       TSharedPtr<SEditableTextBox> ModelFolderPathBox, TSharedPtr<SEditableTextBox> ModelPathBox);
	FReply OnCancelImport(TSharedRef<SWindow> DialogWindow);
	FReply OnOpenJsonFile(TSharedPtr<SEditableTextBox> JsonFilePathBox);
	FReply OnOpenModelFolder(TSharedPtr<SEditableTextBox> ModelFolderPathBox);
};
