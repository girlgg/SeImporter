#include "WraithX/SWraithXWidget.h"

#include "DesktopPlatformModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Localization/IWToUELocalizationManager.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "WraithX/SSettingsDialog.h"

void SWraithXWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.7f)
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(FIWToUELocalizationManager::Get().GetText("SearchBoxHint"))
				.OnTextChanged(this, &SWraithXWidget::HandleSearchChanged)
				.IsEnabled_Lambda([this]() { return !bIsLoading; })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(FIWToUELocalizationManager::Get().GetText("SearchButton"))
				.OnClicked(this, &SWraithXWidget::HandleSearchButton)
				.IsEnabled_Lambda([this]() { return !bIsLoading; })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(FIWToUELocalizationManager::Get().GetText("ClearButton"))
				.OnClicked(this, &SWraithXWidget::HandleClearSearchText)
				.IsEnabled_Lambda([this]() { return !bIsLoading; })
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			[
				SAssignNew(AssetCountText, STextBlock)
				.Justification(ETextJustify::Right)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SComboButton) // 组合按钮带下拉菜单
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(INVTEXT("Filter"))
				]
				.MenuContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SEditableTextBox)
							.HintText(INVTEXT("Asset Name Contains"))
							.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
							{
								// DataManager->SetColumnFilter("AssetName", Text.ToString());
								// DataManager->ApplyFiltersAndSort();
								// ListView->RequestListRefresh();
							})
						]
					]
					// 添加其他列的筛选条件...
				]
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
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(HeaderRow, SHeaderRow)
					+ SHeaderRow::Column(FName("AssetName"))
					.DefaultLabel(FIWToUELocalizationManager::Get().GetText("AssetName"))
					.FillWidth(0.4f)
					.SortMode(this, &SWraithXWidget::GetSortMode, FName("AssetName"))
					.OnSort(this, &SWraithXWidget::OnSortColumnChanged)

					+ SHeaderRow::Column(FName("Status"))
					.DefaultLabel(FIWToUELocalizationManager::Get().GetText("Status"))
					.FillWidth(0.2f)
					.SortMode(this, &SWraithXWidget::GetSortMode, FName("Status"))
					.OnSort(this, &SWraithXWidget::OnSortColumnChanged)

					+ SHeaderRow::Column(FName("Type"))
					.DefaultLabel(FIWToUELocalizationManager::Get().GetText("Type"))
					.FillWidth(0.2f)
					.SortMode(this, &SWraithXWidget::GetSortMode, FName("Type"))
					.OnSort(this, &SWraithXWidget::OnSortColumnChanged)

					+ SHeaderRow::Column(FName("Size"))
					.DefaultLabel(FIWToUELocalizationManager::Get().GetText("Size"))
					.FillWidth(0.2f)
					.SortMode(this, &SWraithXWidget::GetSortMode, FName("Size"))
					.OnSort(this, &SWraithXWidget::OnSortColumnChanged)
				]
				+ SVerticalBox::Slot()
				[
					SAssignNew(ListView, SListView<TSharedPtr<FCoDAsset>>)
					.ListItemsSource(&FilteredItems)
					.OnGenerateRow(this, &SWraithXWidget::GenerateListRow)
					.OnSelectionChanged(this, &SWraithXWidget::HandleSelectionChanged)
					.OnMouseButtonDoubleClick(this, &SWraithXWidget::HandleDoubleClick)
					.SelectionMode(ESelectionMode::Multi)
					.ScrollbarVisibility(EVisibility::Visible)
				]
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
				.FillWidth(1.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SAssignNew(ImportPathInput, SEditableTextBox)
						.HintText(FIWToUELocalizationManager::Get().GetText("ImportPathHint"))
						.IsEnabled_Lambda([this]() { return !bIsLoading; })
						.OnTextCommitted_Lambda([this](const FText& InText, ETextCommit::Type CommitType)
						{
							FString Path = InText.ToString();
							if (!Path.IsEmpty() && !Path.StartsWith("/Game/"))
							{
								FNotificationInfo Info(FIWToUELocalizationManager::Get().GetText("PathErrorHint"));
								Info.ExpireDuration = 3.0f;
								FSlateNotificationManager::Get().AddNotification(Info);
								ImportPathInput->SetText(FText::GetEmpty());
							}
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FIWToUELocalizationManager::Get().GetText("Browse"))
						.OnClicked_Lambda([this]()
						{
							IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
							if (DesktopPlatform)
							{
								TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(
									ImportPathInput.ToSharedRef());
								void* ParentWindowHandle =
									(ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
										? ParentWindow->GetNativeWindow()->GetOSWindowHandle()
										: nullptr;

								FString SelectedPath;
								if (DesktopPlatform->OpenDirectoryDialog(
									ParentWindowHandle,
									*FIWToUELocalizationManager::Get().GetString("BrowseHint"),
									FPaths::ProjectContentDir(),
									SelectedPath))
								{
									if (FPaths::MakePathRelativeTo(SelectedPath, *FPaths::ProjectContentDir()))
									{
										SelectedPath = FString(TEXT("/Game/")) + SelectedPath;
										SelectedPath.RemoveFromEnd(TEXT("/"));
										ImportPathInput->SetText(FText::FromString(SelectedPath));
									}
									else
									{
										FNotificationInfo Info(
											FIWToUELocalizationManager::Get().GetText("BrowseErrorHint"));
										Info.ExpireDuration = 3.0f;
										FSlateNotificationManager::Get().AddNotification(Info);
									}
								}
							}
							return FReply::Handled();
						})
					]
				]
				+ SHorizontalBox::Slot()
				[
					SAssignNew(OptionalParamsInput, SEditableTextBox)
					.HintText(FIWToUELocalizationManager::Get().GetText("OptionalParameters"))
					.IsEnabled_Lambda([this]() { return !bIsLoading; })
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
					.Text(FIWToUELocalizationManager::Get().GetText("LoadGame"))
					.OnClicked(this, &SWraithXWidget::HandleLoadGame)
					.IsEnabled_Lambda([this]() { return !bIsLoading; })
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					.Text(FIWToUELocalizationManager::Get().GetText("RefreshFile"))
					.OnClicked(this, &SWraithXWidget::HandleRefreshGame)
					.IsEnabled_Lambda([this]() { return !bIsLoading; })
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					.Text(FIWToUELocalizationManager::Get().GetText("ImportSelected"))
					.OnClicked(this, &SWraithXWidget::HandleImportSelected)
					.IsEnabled_Lambda([this]() { return !bIsLoading; })
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					.Text(FIWToUELocalizationManager::Get().GetText("ImportAll"))
					.OnClicked(this, &SWraithXWidget::HandleImportAll)
					.IsEnabled_Lambda([this]() { return !bIsLoading; })
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(INVTEXT("Settings"))
					.OnClicked(this, &SWraithXWidget::HandleSettingsButton)
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.FillWidth(0.9f)
			[
				SAssignNew(LoadingIndicator, SProgressBar)
				.Percent(0.0f)
				.Visibility_Lambda([this]() { return bIsLoading ? EVisibility::Visible : EVisibility::Hidden; })
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

	Async(EAsyncExecution::ThreadPool, [this]()
	{
		TArray<TSharedPtr<FCoDAsset>> LocalItems;
		{
			FScopeLock Lock(&DataLock);
			LocalItems = AssetImportManager->GetLoadedAssets();
		}

		TotalAssetCount = LocalItems.Num();

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
			SortData();
			ListView->RequestListRefresh();
			UpdateAssetCount();
		});
	});
}

bool SWraithXWidget::MatchSearchCondition(const TSharedPtr<FCoDAsset>& Item)
{
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
		HandleSearchChanged(FText::FromString(CurrentSearchText));
		UpdateAssetCount();
		ListView->RequestListRefresh();
		OnLoadCompleted();
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
	bIsLoading = true;

	AssetImportManager->RefreshGame();
	AssetImportManager->OnAssetInitCompletedDelegate.AddRaw(this, &SWraithXWidget::LoadInitialAssets);
	AssetImportManager->OnAssetLoadingDelegate.AddRaw(this, &SWraithXWidget::SetLoadingProgress);

	return FReply::Handled();
}

FReply SWraithXWidget::HandleLoadGame()
{
	bIsLoading = true;

	AssetImportManager->InitializeGame();
	if (!AssetImportManager->OnAssetInitCompletedDelegate.IsBound())
		AssetImportManager->OnAssetInitCompletedDelegate.AddRaw(this, &SWraithXWidget::LoadInitialAssets);
	if (!AssetImportManager->OnAssetLoadingDelegate.IsBound())
		AssetImportManager->OnAssetLoadingDelegate.AddRaw(this, &SWraithXWidget::SetLoadingProgress);
	return FReply::Handled();
}

FReply SWraithXWidget::HandleImportSelected()
{
	TArray<TSharedPtr<FCoDAsset>> Selected = ListView->GetSelectedItems();
	FString ImportPath = ImportPathInput->GetText().ToString();
	if (Selected.Num() > 0)
	{
		// TODO：重置底部进度条
		// TODO：关闭所有操作
		// TODO：导入信息与取消导入
		// TODO：异步导入
		AssetImportManager->ImportSelection(ImportPath, Selected);
	}
	return FReply::Handled();
}

FReply SWraithXWidget::HandleImportAll()
{
	UE_LOG(LogTemp, Display, TEXT("Importing all %d items"),
	       FilteredItems.Num());
	return FReply::Handled();
}

FReply SWraithXWidget::HandleSettingsButton()
{
	TSharedRef<SWindow> SettingsWindow = SNew(SWindow)
		.Title(INVTEXT("WraithX 设置"))
		.ClientSize(FVector2D(800, 600))
		.SizingRule(ESizingRule::UserSized);

	SettingsWindow->SetContent(
		SNew(SSettingsDialog)
	);

	FSlateApplication::Get().AddModalWindow(SettingsWindow, FSlateApplication::Get().GetActiveTopLevelWindow());

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

void SWraithXWidget::OnLoadCompleted()
{
	bIsLoading = false;
	LoadingIndicator->SetPercent(1.0f);
}

void SWraithXWidget::SetLoadingProgress(float InProgress)
{
	LoadingIndicator->SetPercent(InProgress);
}

void SWraithXWidget::OnSortColumnChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId,
                                         const EColumnSortMode::Type NewSortMode)
{
	if (CurrentSortColumn != ColumnId)
	{
		CurrentSortMode = EColumnSortMode::Ascending;
	}
	else
	{
		CurrentSortMode = (NewSortMode == EColumnSortMode::Ascending)
			                  ? EColumnSortMode::Descending
			                  : EColumnSortMode::Ascending;
	}

	CurrentSortColumn = ColumnId;
	SortData();
	ListView->RequestListRefresh();
}

EColumnSortMode::Type SWraithXWidget::GetSortMode(const FName ColumnId) const
{
	return (CurrentSortColumn == ColumnId) ? CurrentSortMode : EColumnSortMode::None;
}

void SWraithXWidget::SortData()
{
	if (CurrentSortColumn == FName("AssetName"))
	{
		FilteredItems.Sort([this](const TSharedPtr<FCoDAsset>& A, const TSharedPtr<FCoDAsset>& B)
		{
			return CurrentSortMode == EColumnSortMode::Ascending
				       ? A->AssetName.Compare(B->AssetName) < 0
				       : A->AssetName.Compare(B->AssetName) > 0;
		});
	}
	else if (CurrentSortColumn == FName("Status"))
	{
		FilteredItems.Sort([this](const TSharedPtr<FCoDAsset>& A, const TSharedPtr<FCoDAsset>& B)
		{
			return CurrentSortMode == EColumnSortMode::Ascending
				       ? A->GetAssetStatusText().CompareTo(B->GetAssetStatusText()) < 0
				       : A->GetAssetStatusText().CompareTo(B->GetAssetStatusText()) > 0;
		});
	}
	else if (CurrentSortColumn == FName("Type"))
	{
		FilteredItems.Sort([this](const TSharedPtr<FCoDAsset>& A, const TSharedPtr<FCoDAsset>& B)
		{
			return CurrentSortMode == EColumnSortMode::Ascending
				       ? A->GetAssetTypeText().CompareTo(B->GetAssetTypeText()) < 0
				       : A->GetAssetTypeText().CompareTo(B->GetAssetTypeText()) > 0;
		});
	}
	else if (CurrentSortColumn == FName("Size"))
	{
		FilteredItems.Sort([this](const TSharedPtr<FCoDAsset>& A, const TSharedPtr<FCoDAsset>& B)
		{
			return CurrentSortMode == EColumnSortMode::Ascending
				       ? A->AssetSize < B->AssetSize
				       : A->AssetSize > B->AssetSize;
		});
	}
}
