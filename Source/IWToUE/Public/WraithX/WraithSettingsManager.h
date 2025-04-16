#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

enum class ESettingsCategory
{
	General,
	Advanced,
	Appearance
};

struct FAppearanceSettings
{
	FColor PrimaryColor = FColor(0.2f, 0.2f, 0.8f);
	float UIScale = 1.0f;
	// 添加其他外观设置...
};

struct FGeneralSettings
{
	bool bAutoRefresh = true;
	int32 MaxHistoryItems = 100;
	// 添加其他常规设置...
};

struct FAdvancedSettings
{
	bool bUseParallelProcessing = true;
	int32 CacheSizeMB = 512;
	// 添加其他高级设置...
};

class FWraithSettingsManager
{
public:
	static FWraithSettingsManager& Get();

	// 数据访问接口
	const FGeneralSettings& GetGeneralSettings() const { return GeneralSettings; }
	const FAdvancedSettings& GetAdvancedSettings() const { return AdvancedSettings; }
	const FAppearanceSettings& GetAppearanceSettings() const { return AppearanceSettings; }

	// 带自动保存的修改方法
	void UpdateGeneralSettings(const FGeneralSettings& NewSettings);
	void UpdateAdvancedSettings(const FAdvancedSettings& NewSettings);
	void UpdateAppearanceSettings(const FAppearanceSettings& NewSettings);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSettingsChanged, ESettingsCategory);
	FOnSettingsChanged OnSettingsChanged;

private:
	FWraithSettingsManager();
	void LoadSettings();
	void SaveSettings();

	FGeneralSettings GeneralSettings;
	FAdvancedSettings AdvancedSettings;
	FAppearanceSettings AppearanceSettings;
};
