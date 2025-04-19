#include "WraithX/SWraithXWidget.h"

#include "DesktopPlatformModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Localization/IWToUELocalizationManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "WraithX/SSettingsDialog.h"
#include "WraithX/WraithXViewModel.h"

void SWraithXWidget::Construct(const FArguments& InArgs)
{
	// --- Initialize ViewModel ---
	if (ViewModel.IsValid())
	{
		ViewModel->Initialize(AssetImportManager);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ViewModel is invalid!"));
	}

	// --- Bind View to ViewModel Delegates ---
	// ViewModel->OnLoadingProgressChangedDelegate.BindSP(this, &SWraithXWidget::OnViewModelLoadingProgressChanged);
	// ViewModel->OnLoadingStateChangedDelegate.BindSP(this, &SWraithXWidget::OnViewModelLoadingStateChanged);
	// ViewModel->OnShowNotificationDelegate.BindSP(this, &SWraithXWidget::OnViewModelShowNotification);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			CreateTopToolbar()
		]

		+ SVerticalBox::Slot()
		.FillHeight(0.8f)
		.Padding(4.0f, 8.0f)
		[
			CreateMainArea()
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f, 8.0f)
		[
			CreateBottomPanel()
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateStatusBar()
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
	// CurrentSearchText = NewText.ToString();
	//
	// Async(EAsyncExecution::ThreadPool, [this]()
	// {
	// 	TArray<TSharedPtr<FCoDAsset>> LocalItems;
	// 	{
	// 		FScopeLock Lock(&DataLock);
	// 		LocalItems = AssetImportManager->GetLoadedAssets();
	// 	}
	//
	// 	TotalAssetCount = LocalItems.Num();
	//
	// 	TArray<TSharedPtr<FCoDAsset>> NewFiltered;
	// 	for (const auto& Item : LocalItems)
	// 	{
	// 		if (MatchSearchCondition(Item))
	// 		{
	// 			NewFiltered.Add(Item);
	// 		}
	// 	}
	//
	// 	AsyncTask(ENamedThreads::GameThread, [this, NewFiltered]()
	// 	{
	// 		FScopeLock Lock(&DataLock);
	// 		FilteredItems = NewFiltered;
	// 		SortData();
	// 		ListView->RequestListRefresh();
	// 		UpdateAssetCount();
	// 	});
	// });
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
		AssetImportManager->ImportSelection(ImportPath, Selected, OptionalParamsInput->GetText().ToString());
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

TSharedRef<SWidget> SWraithXWidget::CreateTopToolbar()
{
	const FSlateBrush* ToolbarBg = FAppStyle::GetBrush("ToolPanel.GroupBorder");
	const FButtonStyle& ToolbarButton = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton.Default");

	return SNew(SBorder)
		.BorderImage(ToolbarBg)
		.Padding(8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(FIWToUELocalizationManager::Get().GetText("SearchBoxHint"))
				.OnTextChanged(this, &SWraithXWidget::HandleSearchTextChanged)
				.OnTextCommitted(this, &SWraithXWidget::HandleSearchTextCommitted)
				.IsEnabled(TAttribute<bool>::Create(
					TAttribute<bool>::FGetter::CreateSP(
						this, &SWraithXWidget::IsUIEnabled)))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(&ToolbarButton)
				.OnClicked(this, &SWraithXWidget::HandleClearSearchClicked)
				.IsEnabled(TAttribute<bool>::Create(
					TAttribute<bool>::FGetter::CreateSP(this, &SWraithXWidget::IsUIEnabled)))
				.ToolTipText(NSLOCTEXT("WraithX", "ClearButtonTooltip", "Clear Search Text"))
				[
					SNew(SImage).Image(FAppStyle::GetBrush("Cross"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SAssignNew(AssetCountText, STextBlock)
				.TextStyle(FAppStyle::Get(), "SmallText")
				.Text(NSLOCTEXT("WraithX", "InitialAssetCount", "Total: 0 | Filtered: 0"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(0.1f)
			[
				SNew(SSpacer)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SComboButton)
				.ButtonStyle(&ToolbarButton)
				.HasDownArrow(true)
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Filter"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "SmallText")
						.Text(NSLOCTEXT("WraithX", "FilterButton", "Filters"))
					]
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
				]
				.IsEnabled(TAttribute<bool>::Create(
					TAttribute<bool>::FGetter::CreateSP(this, &SWraithXWidget::IsUIEnabled)))
			]
		];
}

TSharedRef<SWidget> SWraithXWidget::CreateMainArea()
{
	return SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(0.7f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(2.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHeaderRow)

					+ SHeaderRow::Column(FName("AssetName"))
					.DefaultLabel(NSLOCTEXT("WraithX", "AssetName", "Name"))
					.FillWidth(0.4f)
					.SortMode(this, &SWraithXWidget::GetSortMode, FName("AssetName"))
					.OnSort(this, &SWraithXWidget::OnSortColumnChanged)

					+ SHeaderRow::Column(FName("Status"))
					.DefaultLabel(NSLOCTEXT("WraithX", "Status", "Status"))
					.FillWidth(0.2f)
					.SortMode(this, &SWraithXWidget::GetSortMode, FName("Status"))
					.OnSort(this, &SWraithXWidget::OnSortColumnChanged)

					+ SHeaderRow::Column(FName("Type"))
					.DefaultLabel(NSLOCTEXT("WraithX", "Type", "Type"))
					.FillWidth(0.2f)
					.SortMode(this, &SWraithXWidget::GetSortMode, FName("Type"))
					.OnSort(this, &SWraithXWidget::OnSortColumnChanged)

					+ SHeaderRow::Column(FName("Size"))
					.DefaultLabel(NSLOCTEXT("WraithX", "Size", "Size"))
					.FillWidth(0.2f)
					.SortMode(this, &SWraithXWidget::GetSortMode, FName("Size"))
					.OnSort(this, &SWraithXWidget::OnSortColumnChanged)
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FCoDAsset>>)
					.ListItemsSource(&ViewModel->FilteredItems)
					.OnGenerateRow(
						this, &SWraithXWidget::GenerateListRow)
					.OnSelectionChanged(
						this,
						&SWraithXWidget::HandleListSelectionChanged)
					.OnMouseButtonDoubleClick(
						this, &SWraithXWidget::HandleListDoubleClick)
					.SelectionMode(ESelectionMode::Multi)
					.ScrollbarVisibility(EVisibility::Visible)
				]
			]
		]
		+ SSplitter::Slot()
		.Value(0.3f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
					.Text(NSLOCTEXT("WraithX", "AssetDetails", "Asset Details"))
				]
				+ SVerticalBox::Slot()
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(AssetInfoPanel, SAssetInfoPanel)
						.AssetItems(TAttribute<TArray<TSharedPtr<FCoDAsset>>>::Create(
							TAttribute<TArray<TSharedPtr<FCoDAsset>>>::FGetter::CreateLambda([this]()
							{
								return ListView.IsValid()
									       ? ListView->GetSelectedItems()
									       : TArray<TSharedPtr<FCoDAsset>>();
							})))
					]
				]
			]
		];
}

TSharedRef<SWidget> SWraithXWidget::CreateBottomPanel()
{
	const FButtonStyle& ActionButtonStyle = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton");
	const FSlateBrush* PanelBg = FAppStyle::GetBrush("ToolPanel.DarkGroupBorder");

	return SNew(SBorder)
		.BorderImage(PanelBg)
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(0.65f)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.TextStyle(FAppStyle::Get(), "SmallText")
							.Text(FIWToUELocalizationManager::Get().GetText("ImportPathLabel"))
						]
						+ SVerticalBox::Slot()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SAssignNew(ImportPathInput, SEditableTextBox)
								.Style(FAppStyle::Get(), "FlatEditableTextBox")
								.HintText(FIWToUELocalizationManager::Get().GetText("ImportPathHint"))
								.OnTextCommitted(this, &SWraithXWidget::HandleImportPathCommitted)
								.IsEnabled(TAttribute<bool>::Create(
									TAttribute<bool>::FGetter::CreateSP(this, &SWraithXWidget::IsUIEnabled)))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(4.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SButton)
								.ButtonStyle(&ActionButtonStyle)
								.Text(FIWToUELocalizationManager::Get().GetText("Browse"))
								.OnClicked(this, &SWraithXWidget::HandleBrowseImportPathClicked)
								.IsEnabled(TAttribute<bool>::Create(
									TAttribute<bool>::FGetter::CreateSP(this, &SWraithXWidget::IsUIEnabled)))
							]
						]
					]

					+ SHorizontalBox::Slot()
					.FillWidth(0.35f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.TextStyle(FAppStyle::Get(), "SmallText")
							.Text(FIWToUELocalizationManager::Get().GetText("AdvancedSettings"))
						]
						+ SVerticalBox::Slot()
						[
							SNew(SEditableTextBox)
							.HintText(FIWToUELocalizationManager::Get().GetText("OptionalParameters"))
							.IsEnabled(TAttribute<bool>::Create(
								TAttribute<bool>::FGetter::CreateSP(this, &SWraithXWidget::IsUIEnabled)))
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Fill)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(4.0f, 0.0f))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.ButtonStyle(&ActionButtonStyle)
						.Text(FIWToUELocalizationManager::Get().GetText("LoadGame"))
						.OnClicked(this, &SWraithXWidget::HandleLoadGameClicked)
						.IsEnabled(TAttribute<bool>::Create(
							TAttribute<bool>::FGetter::CreateSP(this, &SWraithXWidget::IsUIEnabled)))
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.Text(FIWToUELocalizationManager::Get().GetText("RefreshFile"))
						.OnClicked(this, &SWraithXWidget::HandleRefreshGameClicked)
						.IsEnabled(TAttribute<bool>::Create(
							TAttribute<bool>::FGetter::CreateSP(this, &SWraithXWidget::IsUIEnabled)))
					]
					+ SUniformGridPanel::Slot(2, 0)
					[
						SNew(SButton)
						.OnClicked(this, &SWraithXWidget::HandleImportSelectedClicked)
						.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this]()
						{
							return IsUIEnabled() && ListView.IsValid() && ListView->GetNumItemsSelected() > 0;
						})))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SImage)
								.Image(FAppStyle::Get().GetBrush("Icons.Download"))
								.DesiredSizeOverride(FVector2D(16, 16))
							]
							+ SHorizontalBox::Slot()
							.Padding(4, 0, 0, 0)
							[
								SNew(STextBlock)
								.Text(FIWToUELocalizationManager::Get().GetText("ImportSelected"))
							]
						]
					]
					+ SUniformGridPanel::Slot(3, 0)
					[
						SNew(SButton)
						.OnClicked(this, &SWraithXWidget::HandleImportAllClicked)
						.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this]()
						{
							return IsUIEnabled() && ViewModel.IsValid() && ViewModel->FilteredItems.Num() > 0;
						})))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SImage)
								.Image(FAppStyle::Get().GetBrush("Icons.Download"))
								.DesiredSizeOverride(FVector2D(16, 16))
							]
							+ SHorizontalBox::Slot()
							.Padding(4, 0, 0, 0)
							[
								SNew(STextBlock)
								.Text(FIWToUELocalizationManager::Get().GetText("ImportAll"))
							]
						]
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.OnClicked(this, &SWraithXWidget::HandleSettingsClicked)
					.ToolTipText(NSLOCTEXT("WraithX", "SettingsButtonTooltip", "Open Settings"))
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.Settings"))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
				]
			]
		];
}

TSharedRef<SWidget> SWraithXWidget::CreateStatusBar()
{
	return SNew(SBorder)
		.Padding(FMargin(0.f, 0.f, 0.f, 0.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(LoadingIndicator, SProgressBar)
				.Percent(TAttribute<TOptional<float>>::Create(
					TAttribute<TOptional<float>>::FGetter::CreateLambda(
						[this]()
						{
							return ViewModel.IsValid()
								       ? TOptional<float>(
									       ViewModel->CurrentLoadingProgress)
								       : TOptional<float>(0.0f);
						})))
				.Visibility(TAttribute<EVisibility>::Create(
					TAttribute<EVisibility>::FGetter::CreateLambda(
						[this]()
						{
							return ViewModel.IsValid() && ViewModel->
							       bIsLoading
								       ? EVisibility::Visible
								       : EVisibility::Collapsed;
						})))
			]
		];
}

void SWraithXWidget::HandleSearchTextChanged(const FText& NewText)
{
	if (ViewModel.IsValid())
	{
		ViewModel->SetSearchText(NewText.ToString());
	}
}

void SWraithXWidget::HandleSearchTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (ViewModel.IsValid())
	{
		ViewModel->SetSearchText(NewText.ToString());
	}
}

FReply SWraithXWidget::HandleClearSearchClicked()
{
	if (ViewModel.IsValid())
	{
		SearchBox->SetText(FText::GetEmpty());
		ViewModel->ClearSearchText();
	}
	return FReply::Handled();
}

FReply SWraithXWidget::HandleLoadGameClicked()
{
	if (ViewModel.IsValid())
	{
		ViewModel->LoadGame();
	}
	return FReply::Handled();
}

FReply SWraithXWidget::HandleRefreshGameClicked()
{
	if (ViewModel.IsValid())
	{
		ViewModel->RefreshGame();
	}
	return FReply::Handled();
}

FReply SWraithXWidget::HandleImportSelectedClicked()
{
	if (ViewModel.IsValid())
	{
		ViewModel->ImportSelectedAssets();
	}
	return FReply::Handled();
}

FReply SWraithXWidget::HandleImportAllClicked()
{
	if (ViewModel.IsValid())
	{
		ViewModel->ImportAllAssets();
	}
	return FReply::Handled();
}

FReply SWraithXWidget::HandleSettingsClicked()
{
	TSharedRef<SWindow> SettingsWindow = SNew(SWindow)
	                                                  .Title(NSLOCTEXT("WraithX", "SettingsTitle", "WraithX Settings"))
	                                                  .ClientSize(FVector2D(600, 400)) // Adjust size
	                                                  .SizingRule(ESizingRule::UserSized);

	SettingsWindow->SetContent(SNew(SSettingsDialog));

	FSlateApplication::Get().AddModalWindow(SettingsWindow, FSlateApplication::Get().GetActiveTopLevelWindow());

	return FReply::Handled();
}

FReply SWraithXWidget::HandleBrowseImportPathClicked()
{
	if (ViewModel.IsValid())
	{
		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		ViewModel->BrowseImportPath(ParentWindow);
		ImportPathInput->SetText(FText::FromString(ViewModel->ImportPath));
	}
	return FReply::Handled();
}

void SWraithXWidget::HandleImportPathCommitted(const FText& InText, ETextCommit::Type CommitType)
{
	if (ViewModel.IsValid())
	{
		ViewModel->SetImportPath(InText.ToString());
		ImportPathInput->SetText(FText::FromString(ViewModel->ImportPath));
	}
}

void SWraithXWidget::HandleListSelectionChanged(TSharedPtr<FCoDAsset> Item, ESelectInfo::Type SelectInfo)
{
	if (ViewModel.IsValid() && ListView.IsValid())
	{
		ViewModel->SelectedItems = ListView->GetSelectedItems();
	}
}

void SWraithXWidget::HandleListDoubleClick(TSharedPtr<FCoDAsset> Item)
{
	if (Item.IsValid())
	{
		TSharedRef<SWindow> PreviewWindow = SNew(SWindow)
			.Title(FText::Format(
				NSLOCTEXT("WraithX", "PreviewTitle", "Preview: {0}"), FText::FromString(Item->AssetName)))
			.ClientSize(FVector2D(800, 600));

		FSlateApplication::Get().AddWindow(PreviewWindow);
	}
}

bool SWraithXWidget::IsUIEnabled() const
{
	return ViewModel.IsValid() && !ViewModel->bIsLoading;
}
