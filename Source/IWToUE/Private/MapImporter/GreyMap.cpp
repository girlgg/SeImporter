#include "MapImporter/GreyMap.h"

#include "Widgets/Images/SThrobber.h"

void SGreyMap::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Search for Cordycep.CLI program")))
			.OnClicked(this, &SGreyMap::HandleCheckButtonClicked)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(ErrorText, STextBlock)
			.ColorAndOpacity(FLinearColor::Red)
			.Text(FText::FromString(TEXT("Click the button above to search for the program 'Cordycep.CLI'")))
			.Visibility_Raw(this, &SGreyMap::GetErrorVisibility)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Refresh map list")))
				.OnClicked_Lambda([this]()
				{
					RefreshDataItems();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0)
			[
				SAssignNew(RefreshThrobber, SThrobber)
				.Visibility(EVisibility::Collapsed)
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox)
			.Padding(FMargin(0, 10))
			.Visibility_Raw(this, &SGreyMap::GetInfoBoxVisibility)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Map info: ")))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SAssignNew(InfoTextBlock, STextBlock)
					.ColorAndOpacity(FLinearColor::Green)
					.AutoWrapText(true)
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f)
			[
				SAssignNew(ListView, SListView<TSharedPtr<FCastMapInfo>>)
				.Visibility_Raw(this, &SGreyMap::GetListVisibility)
				.ListItemsSource(&DataItems)
				.OnGenerateRow(this, &SGreyMap::GenerateListRow)
				.OnSelectionChanged(this, &SGreyMap::HandleSelectionChanged)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f)
			[
				SAssignNew(RightPanel, SVerticalBox)
				.Visibility_Raw(this, &SGreyMap::GetRightPanelVisibility)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(DetailText, STextBlock)
					.AutoWrapText(true)
					.Text_Raw(this, &SGreyMap::GetDetailText)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Import selected map")))
					.OnClicked(this, &SGreyMap::HandleProcessButtonClicked)
				]
			]
		]
	];
}

FReply SGreyMap::HandleCheckButtonClicked()
{
	CordycepProcess->Initialize();
	FString ErrorRes = CordycepProcess->GetErrorRes();
	bIsRunning = ErrorRes.IsEmpty();

	if (bIsRunning)
	{
		InfoTextBlock->SetText(FText::FromString(
			FString::Printf(
				TEXT(
					"Cordycep.CLI is running @ %s\nGameID: %s\nPools address: %lld\nStrings address: %lld\nGame directory: %s\nFlags: %s"),
				*CordycepProcess->GetCurrentProcessPath(),
				*CordycepProcess->GetGameID(),
				CordycepProcess->GetPoolsAddress(),
				CordycepProcess->GetStringsAddress(),
				*CordycepProcess->GetGameDirectory(),
				*CordycepProcess->GetFlags())
		));

		RefreshDataItems();

		ListView->RequestListRefresh();
	}
	else
	{
		ErrorText->SetText(FText::FromString(ErrorRes));
		InfoTextBlock->SetText(FText::GetEmpty());
	}

	ErrorText->SetVisibility(GetErrorVisibility());
	return FReply::Handled();
}

TSharedRef<ITableRow> SGreyMap::GenerateListRow(TSharedPtr<FCastMapInfo> Item,
                                                const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FCastMapInfo>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Item->DisplayName))
		];
}

void SGreyMap::HandleSelectionChanged(TSharedPtr<FCastMapInfo> Item, ESelectInfo::Type SelectInfo)
{
	SelectedItem = Item;
	if (SelectedItem.IsValid())
	{
		DetailText->SetText(FText::FromString(SelectedItem->DetailInfo));
	}
	else
	{
		DetailText->SetText(FText::GetEmpty());
	}
}

void SGreyMap::RefreshDataItems()
{
	RefreshThrobber->SetVisibility(EVisibility::Visible);

	SelectedItem = nullptr;
	DetailText->SetText(FText::GetEmpty());

	FPlatformProcess::Sleep(0.2f);

	Async(EAsyncExecution::Thread, [this]()
	{
		TArray<TSharedPtr<FCastMapInfo>> NewItems;

		NewItems = CordycepProcess->GetMapsInfo();

		Async(EAsyncExecution::TaskGraphMainThread, [this, NewItems]()
		{
			DataItems = NewItems;
			RefreshThrobber->SetVisibility(EVisibility::Collapsed);
			ListView->RequestListRefresh();
		});
	});
}

FReply SGreyMap::HandleProcessButtonClicked()
{
	if (SelectedItem.IsValid())
	{
		CordycepProcess->DumpMap(SelectedItem->DisplayName);

		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (ParentWindow.IsValid())
		{
			ParentWindow->RequestDestroyWindow();
		}
	}
	return FReply::Handled();
}
