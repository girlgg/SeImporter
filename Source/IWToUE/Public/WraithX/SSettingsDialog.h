#pragma once

#include "CoreMinimal.h"
#include "SGeneralSettingsPage.h"
#include "WraithSettingsManager.h"
#include "Widgets/SCompoundWidget.h"

namespace WraithXSettings
{
	namespace TabIds
	{
		static const FName General(TEXT("GeneralSettings"));
		static const FName Appearance(TEXT("AppearanceSettings"));
		static const FName Advanced(TEXT("AdvancedSettings"));
	}

	// 布局配置版本控制
	static const FName LayoutVersion(TEXT("SettingsLayout_v1"));
}

class IWTOUE_API SSettingsDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSettingsDialog)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		// 1. 创建标签管理器（不关联特定DockTab）
		TSharedRef<SDockTab> DummyOwnerTab = SNew(SDockTab)
			.TabRole(ETabRole::PanelTab);

		// 正确创建TabManager（必须传入OwnerTab）
		TabManager = FGlobalTabmanager::Get()->NewTabManager(DummyOwnerTab);

		// 2. 注册标签页生成器
		RegisterTabSpawners();

		// 3. 初始化布局
		const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(WraithXSettings::LayoutVersion)
			->AddArea(
				FTabManager::NewPrimaryArea()
				->Split(
					FTabManager::NewStack()
					->AddTab(WraithXSettings::TabIds::General, ETabState::OpenedTab)
					->AddTab(WraithXSettings::TabIds::Appearance, ETabState::OpenedTab)
					->AddTab(WraithXSettings::TabIds::Advanced, ETabState::OpenedTab)
					->SetForegroundTab(WraithXSettings::TabIds::General)
				)
			);

		// 4. 构建主界面
		ChildSlot
		[
			SNew(SVerticalBox)

			// 标签内容区域
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				TabManager->RestoreFrom(Layout, nullptr).ToSharedRef()
			]

			// 底部按钮栏
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5)
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.Text(INVTEXT("关闭"))
				.OnClicked(this, &SSettingsDialog::CloseDialog)
			]
		];
	}

private:
	void RegisterTabSpawners()
	{
		TabManager->RegisterTabSpawner(WraithXSettings::TabIds::General,
		                               FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs& Args)
		                               {
			                               return SNew(SDockTab)
				                               .Label(INVTEXT("常规设置"))
				                               [
					                               SNew(SGeneralSettingsPage)
				                               ];
		                               }))
		          .SetDisplayName(INVTEXT("常规设置"));

		// 同理注册其他标签...
	}

	FReply CloseDialog()
	{
		if (TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()))
		{
			ParentWindow->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	TSharedPtr<FTabManager> TabManager;
};
