#pragma once
#include "CoDAssetType.h"
#include "WraithX.h"

class SProgressBar;

class SWraithXPreviewWindow final : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SWraithXPreviewWindow)
		{
		}

		SLATE_ARGUMENT(FString, FilePath)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SWindow::Construct(SWindow::FArguments()
		                   .Title(FText::FromString("Preview - " + InArgs._FilePath))
		                   .ClientSize(FVector2D(800, 600)));

		// 加载并显示预览内容
		// ...
	}
};

class SAssetInfoPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAssetInfoPanel)
		{
		}

		SLATE_ATTRIBUTE(TArray<TSharedPtr<FCoDAsset>>, AssetItems)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		AssetItems = InArgs._AssetItems;

		ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(this, &SAssetInfoPanel::GetSummaryText)
				.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.Text(this, &SAssetInfoPanel::GetDetailsText)
					.AutoWrapText(true)
				]
			]
		];
	}

private:
	FText GetSummaryText() const
	{
		auto Items = AssetItems.Get();
		if (Items.Num() == 0)
		{
			return INVTEXT("No selection");
		}
		if (Items.Num() == 1)
		{
			return FText::Format(INVTEXT("Selected: {0}"),
			                     FText::FromString(Items[0]->AssetName));
		}
		return FText::Format(INVTEXT("Selected {0} items"),
		                     FText::AsNumber(Items.Num()));
	}

	// 细节面板
	FText GetDetailsText() const
	{
		TStringBuilder<1024> Details;
		auto Items = AssetItems.Get();

		if (Items.Num() == 1)
		{
			const auto& Item = Items[0];
			Details.Appendf(
				TEXT("File Name: %s\n")
				TEXT("Type: %s\n")
				TEXT("Status: %s\n")
				TEXT("Hash: %lld KB"),
				*Item->AssetName,
				*Item->GetAssetTypeText().ToString(),
				*Item->GetAssetStatusText().ToString(),
				Item->AssetSize / 1024
			);
		}
		else if (Items.Num() > 1)
		{
			/*TMap<FName, int32> TypeCounts;
			for (const auto& Item : Items)
			{
				TypeCounts.FindOrAdd()++;
			}

			Details.Appendf(TEXT("Total Items: %d\n"), Items.Num());
			for (const auto& Pair : TypeCounts)
			{
				Details.Appendf(TEXT("%s: %d\n"),
				                *Pair.Key.ToString(), Pair.Value);
			}*/
		}
		return FText::FromString(Details.ToString());
	}

	TAttribute<TArray<TSharedPtr<FCoDAsset>>> AssetItems;
};

class SWraithXWidget final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWraithXWidget)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<ITableRow> GenerateListRow(TSharedPtr<FCoDAsset> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleSearchChanged(const FText& NewText);
	bool MatchSearchCondition(const TSharedPtr<FCoDAsset>& Item);
	void LoadInitialAssets();
	void HandleDoubleClick(TSharedPtr<FCoDAsset> Item);
	FReply HandleClearSearchText();
	FReply HandleSearchButton();
	void HandleSelectionChanged(TSharedPtr<FCoDAsset> Item, ESelectInfo::Type SelectInfo);
	FReply HandleRefreshGame();
	FReply HandleLoadGame();
	FReply HandleImportSelected();
	FReply HandleImportAll();
	FReply HandleSettingsButton();
	void UpdateAssetCount();
	void OnLoadCompleted();

	void AddLoadingProgress(float InProgress);

	void OnSortColumnChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type NewSortMode);
	EColumnSortMode::Type GetSortMode(const FName ColumnId) const;
	void SortData();

private:
	// 数据源
	TArray<TSharedPtr<FCoDAsset>> FilteredItems{};

	// UI组件
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SListView<TSharedPtr<FCoDAsset>>> ListView;
	TSharedPtr<STextBlock> AssetCountText;
	TSharedPtr<SEditableTextBox> ImportPathInput;
	TSharedPtr<SEditableTextBox> OptionalParamsInput;
	TSharedPtr<SProgressBar> LoadingIndicator;

	// 搜索处理
	FString CurrentSearchText;
	FThreadSafeBool bSearchRequested = false;

	TSharedPtr<SHeaderRow> HeaderRow;
	FName CurrentSortColumn;
	EColumnSortMode::Type CurrentSortMode = EColumnSortMode::None;

	int32 TotalAssetCount = 0;
	FCriticalSection DataLock;

	TUniquePtr<FWraithX> WraithX = MakeUnique<FWraithX>();

	float CurrentLoadingProgress = 0.f;
	bool bIsLoading = false;
};
