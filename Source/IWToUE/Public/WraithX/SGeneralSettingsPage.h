#pragma once

#include "CoreMinimal.h"
#include "SSettingsPage.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "WraithX/WraithSettingsManager.h"


class IWTOUE_API SGeneralSettingsPage : public SSettingsPage
{
protected:
	virtual TSharedRef<SWidget> BuildLayout() override
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SCheckBox)
				.IsChecked(this, &SGeneralSettingsPage::IsAutoRefreshChecked)
				.OnCheckStateChanged(this, &SGeneralSettingsPage::HandleAutoRefreshChanged)
				.Content()
				[
					SNew(STextBlock).Text(INVTEXT("启用自动刷新"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SNumericEntryBox<int32>)
				.Value(this, &SGeneralSettingsPage::GetMaxHistoryItems)
				.OnValueChanged(this, &SGeneralSettingsPage::SetMaxHistoryItems)
				.Label()
				[
					SNew(STextBlock).Text(INVTEXT("最大历史记录数"))
				]
			];
	}

private:
	ECheckBoxState IsAutoRefreshChecked() const
	{
		return CurrentSettings.bAutoRefresh ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	void HandleAutoRefreshChanged(ECheckBoxState NewState)
	{
		const bool bEnabled = (NewState == ECheckBoxState::Checked);
		// FWraithSettingsManager::Get().UpdateGeneralSettings([&](FGeneralSettings& Settings)
		// {
		// 	Settings.bAutoRefresh = bEnabled;
		// });
	}

	TOptional<int32> GetMaxHistoryItems() const { return CurrentSettings.MaxHistoryItems; }

	void SetMaxHistoryItems(int32 NewValue)
	{
		// FWraithSettingsManager::Get().UpdateGeneralSettings([&](FGeneralSettings& Settings)
		// {
		// 	Settings.MaxHistoryItems = FMath::Clamp(NewValue, 10, 1000);
		// });
	}
};
