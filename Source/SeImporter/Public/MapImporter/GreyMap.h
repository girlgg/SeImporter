#pragma once

#include "CoreMinimal.h"
#include "CordycepProcess.h"
#include "UObject/Object.h"

DECLARE_DELEGATE_OneParam(FOnMapSelected, const FCastMapInfo&);

class SGreyMap : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGreyMap)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply HandleCheckButtonClicked();
	FReply HandleProcessButtonClicked();
	TSharedRef<ITableRow> GenerateListRow(TSharedPtr<FCastMapInfo> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleSelectionChanged(TSharedPtr<FCastMapInfo> Item, ESelectInfo::Type SelectInfo);
	
	void RefreshDataItems();

	TArray<TSharedPtr<FCastMapInfo>> DataItems;
	TSharedPtr<SThrobber> RefreshThrobber;
	TSharedPtr<SListView<TSharedPtr<FCastMapInfo>>> ListView;
	TSharedPtr<STextBlock> ErrorText;
	TSharedPtr<STextBlock> DetailText;
	TSharedPtr<STextBlock> InfoTextBlock;
	TSharedPtr<SWidget> RightPanel;
	TSharedPtr<FCastMapInfo> SelectedItem;
	bool bIsRunning{false};

	EVisibility GetErrorVisibility() const { return bIsRunning ? EVisibility::Collapsed : EVisibility::Visible; }
	EVisibility GetListVisibility() const { return bIsRunning ? EVisibility::Visible : EVisibility::Collapsed; }
	EVisibility GetInfoBoxVisibility() const { return bIsRunning ? EVisibility::Visible : EVisibility::Collapsed; }

	EVisibility GetRightPanelVisibility() const
	{
		return (bIsRunning && SelectedItem.IsValid()) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	FText GetDetailText() const
	{
		return SelectedItem.IsValid() ? FText::FromString(SelectedItem->DetailInfo) : FText::GetEmpty();
	}

	TUniquePtr<FCordycepProcess> CordycepProcess{MakeUnique<FCordycepProcess>()};
};
