// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WraithSettingsManager.h"
#include "Widgets/SCompoundWidget.h"

// template<typename TSettings>
class IWTOUE_API SSettingsPage : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSettingsPage)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		CurrentSettings = FWraithSettingsManager::Get().GetGeneralSettings();
		// RegisterHandlers();
		BuildLayout();
	}

protected:
	virtual TSharedRef<SWidget> BuildLayout() = 0;
	// virtual void RegisterHandlers() = 0;

	TSharedPtr<FSlateStyleSet> StyleSet;
	FGeneralSettings CurrentSettings;

	template <typename TValue>
	using TSettingHandler = TFunction<void(TValue)>;

	template <typename TValue>
	void CreateAutoSaveBinding(
		TValue FGeneralSettings::* MemberPtr,
		TSettingHandler<TValue> Handler = nullptr)
	{
		auto Delegate = [this, MemberPtr, Handler](TValue NewValue)
		{
			CurrentSettings.*MemberPtr = NewValue;
			FWraithSettingsManager::Get().UpdateGeneralSettings(CurrentSettings);
			if (Handler) Handler(NewValue);
		};
		// 返回可用于Slate控件的绑定委托...
	}
};
