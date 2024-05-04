#include "SSeModelImportOptions.h"

#include "UserMeshOptions.h"
#include "Structures/SeModel.h"
#include "Structures/SeModelHeader.h"

#define LOCTEXT_NAMESPACE "PSAImportFac"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSeModelImportOptions::Construct(const FArguments& InArgs)
{
	MeshHeader = InArgs._MeshHeader;
	WidgetWindow = InArgs._WidgetWindow;
	
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	const TSharedRef<IDetailsView> Details = EditModule.CreateDetailView(DetailsViewArgs);
	EditModule.CreatePropertyTable();
	UObject* Container = NewObject<UUserMeshOptions>();
	Options = Cast<UUserMeshOptions>(Container);
	Details->SetObject(Container);
	TSharedPtr<SBox> ImportMapHeaderDisplay;
	Details->SetEnabled(true);

	this->ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
		.Padding(10)
		[
			SNew(SVerticalBox)

			// ImportMapHeaderDisplay
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(ImportMapHeaderDisplay, SBox)
			]

			// Details
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

			// Apply/Apply to All/Cancel Buttons
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				// Apply Button
				+ SHorizontalBox::Slot()
				  .HAlign(HAlign_Right)
				  .Padding(2)
				[
					SNew(SButton)
					.Text(LOCTEXT("Import", "Apply"))
					.OnClicked(this, &SSeModelImportOptions::OnImport)
				]

				// Apply to All Button
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(2)
				[
					SNew(SButton)
					.Text(LOCTEXT("ImportAll", "Apply to All"))
					.OnClicked(this, &SSeModelImportOptions::OnImportAll)
				]

				// Cancel Button
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(2)
				[
					SNew(SButton)
					.Text(LOCTEXT("Cancel", "Cancel"))
					.OnClicked(this, &SSeModelImportOptions::OnCancel)
				]
			]
		]
	];

	ImportMapHeaderDisplay->SetContent(CreateMapHeader(MeshHeader).ToSharedRef());
}

TSharedPtr<SBorder> SSeModelImportOptions::CreateMapHeader(SeModel* MeshHeader)
{
	check(MeshHeader);
	return SNew(SBorder)
		.Padding(FMargin(3))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
	[
		// Add map name to our info display
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			AddNewHeaderProperty(FText::FromString("Model: "), FText::FromString(MeshHeader->Header->MeshName)).
			ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			AddNewHeaderProperty(FText::FromString("Vertices:"), FText::AsNumber(MeshHeader->SurfaceVertCounter)).
			ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			AddNewHeaderProperty(FText::FromString("Materials:"),
			                     FText::AsNumber(MeshHeader->Header->HeaderMaterialCount)).ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			AddNewHeaderProperty(FText::FromString("Bones:"), FText::AsNumber(MeshHeader->Header->HeaderBoneCount)).
			ToSharedRef()
		]
	];
}

/* Generate new slate line for our map info display
 * @InKey - Name of the property
 * @InValue - Value of the property*/
TSharedPtr<SHorizontalBox> SSeModelImportOptions::AddNewHeaderProperty(FText InKey, FText InValue)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
					.Font(FAppStyle::GetFontStyle("CurveEd.LabelFont"))
					.Text(InKey)
		]
		+ SHorizontalBox::Slot()
		  //.Padding(50 - InKey.ToString().Len(), 0, 0, 0)
		  //.AutoWidth()
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Right)
		[
			SNew(STextBlock)
					.Font(FAppStyle::GetFontStyle("CurveEd.InfoFont"))
					.Text(InValue)
		];
}

bool SSeModelImportOptions::ShouldImport()
{
	return UserDlgResponse == EPSAImportOptionDlgResponse::Import;
}

bool SSeModelImportOptions::ShouldImportAll()
{
	return UserDlgResponse == EPSAImportOptionDlgResponse::ImportAll;
}

bool SSeModelImportOptions::ShouldCancel()
{
	return UserDlgResponse == EPSAImportOptionDlgResponse::Cancel;
}

FReply SSeModelImportOptions::OnImportAll()
{
	UserDlgResponse = EPSAImportOptionDlgResponse::ImportAll;
	return HandleImport();
}

FReply SSeModelImportOptions::OnImport()
{
	UserDlgResponse = EPSAImportOptionDlgResponse::Import;
	return HandleImport();
}

FReply SSeModelImportOptions::OnCancel()
{
	UserDlgResponse = EPSAImportOptionDlgResponse::Cancel;
	return FReply::Handled();
}

FReply SSeModelImportOptions::HandleImport()
{
	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
