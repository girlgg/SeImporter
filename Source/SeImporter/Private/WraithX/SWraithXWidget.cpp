#include "WraithX/SWraithXWidget.h"
#include "Widgets/Input/SSearchBox.h"

void SWraithXWidget::Construct(const FArguments& InArgs)
{
	// 主垂直布局
	ChildSlot
	[
		SNew(SVerticalBox)

		// 顶部工具栏
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.7f)
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(INVTEXT("Search assets..."))
				.OnTextChanged(this, &SWraithXWidget::HandleSearchChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(INVTEXT("Search"))
				.OnClicked(this, &SWraithXWidget::HandleSearchButton)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(INVTEXT("Clear"))
				.OnClicked(this, &SWraithXWidget::HandleClearSearchText)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			[
				SAssignNew(AssetCountText, STextBlock)
				.Justification(ETextJustify::Right)
			]
		]

		// 中间区域
		+ SVerticalBox::Slot()
		.FillHeight(0.8f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			+ SSplitter::Slot()
			.Value(0.7f)
			[
				SAssignNew(ListView, SListView<TSharedPtr<FCoDAsset>>)
				.ListItemsSource(&FilteredItems)
				.OnGenerateRow(this, &SWraithXWidget::GenerateListRow)
				.OnSelectionChanged(this, &SWraithXWidget::HandleSelectionChanged)
				.OnMouseButtonDoubleClick(this, &SWraithXWidget::HandleDoubleClick)
				.SelectionMode(ESelectionMode::Multi)
				.ScrollbarVisibility(EVisibility::Visible)
			]

			+ SSplitter::Slot()
			.Value(0.3f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SAssetInfoPanel)
					.AssetItems_Lambda([this]() { return ListView->GetSelectedItems(); })
				]
			]
		]

		// 底部面板
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SVerticalBox)

			// 导入参数
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SAssignNew(ImportPathInput, SEditableTextBox)
					.HintText(INVTEXT("Import Path"))
				]
				+ SHorizontalBox::Slot()
				[
					SAssignNew(OptionalParamsInput, SEditableTextBox)
					.HintText(INVTEXT("Optional Parameters"))
				]
			]

			// 操作按钮
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					.Text(INVTEXT("Load Game"))
					.OnClicked(this, &SWraithXWidget::HandleLoadGame)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					.Text(INVTEXT("Refresh File"))
					.OnClicked(this, &SWraithXWidget::HandleRefreshGame)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					.Text(INVTEXT("Import Selected"))
					.OnClicked(this, &SWraithXWidget::HandleImportSelected)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					.Text(INVTEXT("Import All"))
					.OnClicked(this, &SWraithXWidget::HandleImportAll)
				]
			]
		]
	];
}

TSharedRef<ITableRow> SWraithXWidget::GenerateListRow(TSharedPtr<FCoDAsset> Item,
                                                      const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FCoDAsset>>, OwnerTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.4f)
			[
				SNew(STextBlock).Text(FText::FromString(Item->AssetName))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.2f)
			[
				SNew(STextBlock).Text(Item->GetAssetStatusText())
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.2f)
			[
				SNew(STextBlock).Text(Item->GetAssetTypeText())
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.2f)
			[
				SNew(STextBlock).Text(FText::AsNumber(Item->AssetSize))
			]
		];
}

void SWraithXWidget::HandleSearchChanged(const FText& NewText)
{
	CurrentSearchText = NewText.ToString();

	// 使用Async替代直接使用线程池
	Async(EAsyncExecution::ThreadPool, [this]()
	{
		TArray<TSharedPtr<FCoDAsset>> LocalItems;
		{
			FScopeLock Lock(&DataLock);
			LocalItems = WraithX->GetLoadedAssets();
		}

		TArray<TSharedPtr<FCoDAsset>> NewFiltered;
		for (const auto& Item : LocalItems)
		{
			if (MatchSearchCondition(Item))
			{
				NewFiltered.Add(Item);
			}
		}

		AsyncTask(ENamedThreads::GameThread, [this, NewFiltered]()
		{
			FScopeLock Lock(&DataLock);
			FilteredItems = NewFiltered;
			ListView->RequestListRefresh();
			UpdateAssetCount();
		});
	});
}

bool SWraithXWidget::MatchSearchCondition(const TSharedPtr<FCoDAsset>& Item)
{
	// 实现基于表达式的搜索逻辑
	// 示例支持简单的AND逻辑： "text1 text2" -> 同时包含text1和text2
	TArray<FString> Tokens;
	CurrentSearchText.ParseIntoArray(Tokens, TEXT(" "), true);

	for (const FString& Token : Tokens)
	{
		if (!Item->AssetName.Contains(Token))
		{
			return false;
		}
	}
	return true;
}

void SWraithXWidget::LoadInitialAssets()
{
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		HandleSearchChanged(FText::FromString(TEXT("")));
		UpdateAssetCount();
		ListView->RequestListRefresh();
	});
}

void SWraithXWidget::HandleDoubleClick(TSharedPtr<FCoDAsset> Item)
{
	TSharedRef<SWraithXPreviewWindow> PreviewWindow = SNew(SWraithXPreviewWindow)
		.FilePath(Item->AssetName);

	FSlateApplication::Get().AddWindow(PreviewWindow);
}

FReply SWraithXWidget::HandleClearSearchText()
{
	SearchBox->SetText(FText());
	return FReply::Handled();
}

FReply SWraithXWidget::HandleSearchButton()
{
	HandleSearchChanged(FText::FromString(CurrentSearchText));
	return FReply::Handled();
}

void SWraithXWidget::HandleSelectionChanged(TSharedPtr<FCoDAsset> Item, ESelectInfo::Type SelectInfo)
{
	UpdateAssetCount();
}

FReply SWraithXWidget::HandleRefreshGame()
{
	FPlatformProcess::LaunchURL(
		TEXT("com.yourengine.yourgame"),
		nullptr,
		nullptr);
	return FReply::Handled();
}

FReply SWraithXWidget::HandleLoadGame()
{
	WraithX->InitializeGame();
	WraithX->OnOnAssetInitCompletedDelegate.AddRaw(this, &SWraithXWidget::LoadInitialAssets);
	return FReply::Handled();
}

FReply SWraithXWidget::HandleImportSelected()
{
	TArray<TSharedPtr<FCoDAsset>> Selected = ListView->GetSelectedItems();
	UE_LOG(LogTemp, Display, TEXT("Importing %d items to: %s"),
	       Selected.Num(),
	       *ImportPathInput->GetText().ToString());
	return FReply::Handled();
}

FReply SWraithXWidget::HandleImportAll()
{
	UE_LOG(LogTemp, Display, TEXT("Importing all %d items"),
	       FilteredItems.Num());
	return FReply::Handled();
}

void SWraithXWidget::UpdateAssetCount()
{
	FScopeLock Lock(&DataLock);
	AssetCountText->SetText(
		FText::Format(INVTEXT("Total: {0} | Filtered: {1}"),
		              TotalAssetCount,
		              FilteredItems.Num()));
}
