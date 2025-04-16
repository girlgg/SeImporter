#pragma once

#include "CoreMinimal.h"

class FIWToUELocalizationManager
{
public:
	static FIWToUELocalizationManager& Get();

	/*! 
	 * @return 获取本地化文本 
	 */
	FString GetString(const FString& Key, const FString& NotFoundText = TEXT("LOC_TEXT_NOT_FOUND")) const;
	FText GetText(const FString& Key, const FString& NotFoundText = TEXT("LOC_TEXT_NOT_FOUND")) const;

private:
	FIWToUELocalizationManager();

	/*!
	 * 加载本地化数据
	 */
	void LoadLocalizationData();

	/*!
	 * @return 获取当前语言代码 
	 */
	FString GetCurrentLanguageCode() const;

	/*!
	 * @return 本地化文件路径
	 */
	FString GetLocalizationFilePath(const FString& LanguageCode) const;
	void GenerateLanguageFallbackChain(const FString& LanguageCode, TArray<FString>& OutChain) const;

	TMap<FString, FString> LocalizationMap;
};
