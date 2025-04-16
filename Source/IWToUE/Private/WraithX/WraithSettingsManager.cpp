#include "WraithX/WraithSettingsManager.h"

FWraithSettingsManager& FWraithSettingsManager::Get()
{
	static FWraithSettingsManager Instance;
	return Instance;
}

void FWraithSettingsManager::UpdateGeneralSettings(const FGeneralSettings& NewSettings)
{
	GeneralSettings = NewSettings;
	SaveSettings();
	OnSettingsChanged.Broadcast(ESettingsCategory::General);
}

void FWraithSettingsManager::UpdateAdvancedSettings(const FAdvancedSettings& NewSettings)
{
}

void FWraithSettingsManager::UpdateAppearanceSettings(const FAppearanceSettings& NewSettings)
{
}

FWraithSettingsManager::FWraithSettingsManager()
{
	LoadSettings();
}

void FWraithSettingsManager::LoadSettings()
{
	const FString ConfigPath = FPaths::ProjectConfigDir() / TEXT("IWToUESettings.ini");
    
	// 加载常规设置
	GeneralSettings.bAutoRefresh = GConfig->GetBool(TEXT("General"), TEXT("bAutoRefresh"), GeneralSettings.bAutoRefresh, ConfigPath);
	GeneralSettings.MaxHistoryItems = GConfig->GetInt(TEXT("General"), TEXT("MaxHistoryItems"), GeneralSettings.MaxHistoryItems, ConfigPath);

	// 加载高级设置
	AdvancedSettings.bUseParallelProcessing = GConfig->GetBool(TEXT("Advanced"), TEXT("bUseParallelProcessing"), AdvancedSettings.bUseParallelProcessing, ConfigPath);
	AdvancedSettings.CacheSizeMB = GConfig->GetInt(TEXT("Advanced"), TEXT("CacheSizeMB"), AdvancedSettings.CacheSizeMB, ConfigPath);

	// 加载外观设置
	GConfig->GetColor(TEXT("Appearance"), TEXT("PrimaryColor"), AppearanceSettings.PrimaryColor, ConfigPath);
	GConfig->GetFloat(TEXT("Appearance"), TEXT("UIScale"), AppearanceSettings.UIScale, ConfigPath);
}

void FWraithSettingsManager::SaveSettings()
{
	const FString ConfigPath = FPaths::ProjectConfigDir() / TEXT("IWToUESettings.ini");

	// 保存常规设置
	GConfig->SetBool(TEXT("General"), TEXT("bAutoRefresh"), GeneralSettings.bAutoRefresh, ConfigPath);
	GConfig->SetInt(TEXT("General"), TEXT("MaxHistoryItems"), GeneralSettings.MaxHistoryItems, ConfigPath);

	// 保存高级设置
	GConfig->SetBool(TEXT("Advanced"), TEXT("bUseParallelProcessing"), AdvancedSettings.bUseParallelProcessing, ConfigPath);
	GConfig->SetInt(TEXT("Advanced"), TEXT("CacheSizeMB"), AdvancedSettings.CacheSizeMB, ConfigPath);

	// 保存外观设置
	GConfig->SetColor(TEXT("Appearance"), TEXT("PrimaryColor"), AppearanceSettings.PrimaryColor, ConfigPath);
	GConfig->SetFloat(TEXT("Appearance"), TEXT("UIScale"), AppearanceSettings.UIScale, ConfigPath);

	GConfig->Flush(false, ConfigPath);
}
