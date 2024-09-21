// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SSeAnimImportOption.h"

#include "SeAnimOptions.h"

#define LOCTEXT_NAMESPACE "SeImportFac"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSeAnimImportOption::Construct(const FArguments& InArgs)
{
	WidgetWindow = InArgs._WidgetWindow;
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	TSharedRef<IDetailsView> Details = EditModule.CreateDetailView(DetailsViewArgs);
	EditModule.CreatePropertyTable();
	UObject* Container = NewObject<USeAnimOptions>();
	Stun = Cast<USeAnimOptions>(Container);
	Details->SetObject(Container);
	Details->SetEnabled(true);


	this->ChildSlot
	[
		SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
		.Padding(10)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.Padding(FMargin(3))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
				]
			]

			// Data row struct
			// Curve interpolation
			// Details panel
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(2)
			[
				SNew(SBox)
				.WidthOverride(400)
				[
					Details
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .HAlign(HAlign_Right)
				  .Padding(2)
				[
					SNew(SButton)
			.Text(LOCTEXT("Import", "Apply"))
		.OnClicked(this, &SSeAnimImportOption::OnImport)
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(2)
				[
					SNew(SButton)
			.Text(LOCTEXT("ImportAll", "Apply to All"))
		.OnClicked(this, &SSeAnimImportOption::OnImportAll)
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(2)
				[
					SNew(SButton)
			.Text(LOCTEXT("Cancel", "Cancel"))
		.OnClicked(this, &SSeAnimImportOption::OnCancel)
				]
			]
		]
		// Apply/Apply to All/Cancel
	];
}

bool SSeAnimImportOption::ShouldImport()
{
	return UserDlgResponse == EImportOptionDlgResponse::Import;
}

bool SSeAnimImportOption::ShouldImportAll()
{
	return UserDlgResponse == EImportOptionDlgResponse::ImportAll;
}

FReply SSeAnimImportOption::OnImportAll()
{
	UserDlgResponse = EImportOptionDlgResponse::ImportAll;
	return HandleImport();
}

FReply SSeAnimImportOption::OnImport()
{
	UserDlgResponse = EImportOptionDlgResponse::Import;
	return HandleImport();
}

FReply SSeAnimImportOption::OnCancel()
{
	UserDlgResponse = EImportOptionDlgResponse::Cancel;
	return FReply::Handled();
}

FReply SSeAnimImportOption::HandleImport()
{
	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE
