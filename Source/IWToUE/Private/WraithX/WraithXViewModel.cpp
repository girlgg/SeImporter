#include "WraithX/WraithXViewModel.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "AssetImporter/AssetImportManager.h"
#include "WraithX/CoDAssetType.h"

FName EWraithXSortColumnToFName(EWraithXSortColumn Column)
{
	switch (Column)
	{
	case EWraithXSortColumn::AssetName: return FName("AssetName");
	case EWraithXSortColumn::Status: return FName("Status");
	case EWraithXSortColumn::Type: return FName("Type");
	case EWraithXSortColumn::Size: return FName("Size");
	default: return NAME_None;
	}
}

EWraithXSortColumn FNameToEWraithXSortColumn(FName Name)
{
	if (Name == FName("AssetName")) return EWraithXSortColumn::AssetName;
	if (Name == FName("Status")) return EWraithXSortColumn::Status;
	if (Name == FName("Type")) return EWraithXSortColumn::Type;
	if (Name == FName("Size")) return EWraithXSortColumn::Size;
	return EWraithXSortColumn::None;
}

FWraithXViewModel::FWraithXViewModel()
{
	ImportPath = TEXT("/Game/ImportedAssets/");
}

FWraithXViewModel::~FWraithXViewModel()
{
	Cleanup();
}

void FWraithXViewModel::Initialize(TSharedPtr<FAssetImportManager> InAssetImportManager)
{
	if (!InAssetImportManager.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FWraithXViewModel::Initialize - Invalid FAssetImportManager provided!"));
		return;
	}

	Cleanup();
	
	AssetImportManager = InAssetImportManager;

	AssetImportManager->OnAssetInitCompletedDelegate.AddSP(AsShared(), &FWraithXViewModel::HandleAssetInitCompleted);
	AssetImportManager->OnAssetLoadingDelegate.AddSP(AsShared(), &FWraithXViewModel::HandleAssetLoadingProgress);
}

void FWraithXViewModel::Cleanup()
{
	CancelCurrentAsyncTask();

	if (AssetImportManager.IsValid())
	{
		AssetImportManager->OnAssetInitCompletedDelegate.RemoveAll(this);
		AssetImportManager->OnAssetLoadingDelegate.RemoveAll(this);
	}
	AssetImportManager.Reset();
}

const TArray<TSharedPtr<FCoDAsset>>& FWraithXViewModel::GetFilteredItems() const
{
	return FilteredItems;
}

float FWraithXViewModel::GetLoadingProgress() const
{
	return CurrentLoadingProgress;
}

bool FWraithXViewModel::IsLoading() const
{
	return bIsLoading;
}

int32 FWraithXViewModel::GetTotalAssetCount() const
{
	return TotalAssetCount;
}

int32 FWraithXViewModel::GetFilteredAssetCount() const
{
	return FilteredItems.Num();
}

const FString& FWraithXViewModel::GetImportPath() const
{
	return ImportPath;
}

const FString& FWraithXViewModel::GetOptionalParams() const
{
	return OptionalParams;
}

EColumnSortMode::Type FWraithXViewModel::GetSortModeForColumn(FName ColumnId) const
{
	return (CurrentSortColumn == FNameToEWraithXSortColumn(ColumnId)) ? CurrentSortMode : EColumnSortMode::None;
}

const FString& FWraithXViewModel::GetSearchText() const
{
	return CurrentSearchText;
}

void FWraithXViewModel::SetSearchText(const FString& InText)
{
	if (CurrentSearchText != InText)
	{
		CurrentSearchText = InText;
		ApplyFilteringAndSortingAsync();
	}
}

void FWraithXViewModel::ClearSearchText()
{
	SetSearchText(FString());
}

void FWraithXViewModel::LoadGame()
{
	if (bIsLoading || !AssetImportManager.IsValid()) return;

	SetLoadingState(true);
	UpdateLoadingProgress(0.0f);
	AllLoadedAssets.Empty();
	FilteredItems.Empty();
	TotalAssetCount = 0;
	ExecuteDelegateOnGameThread(OnListChangedDelegate);
	ExecuteDelegateOnGameThread(OnAssetCountChangedDelegate, TotalAssetCount, 0);

	AssetImportManager->InitializeGame();
}

void FWraithXViewModel::RefreshGame()
{
	if (bIsLoading || !AssetImportManager.IsValid()) return;

	SetLoadingState(true);
	UpdateLoadingProgress(0.0f);
	AllLoadedAssets.Empty();
	FilteredItems.Empty();
	TotalAssetCount = 0;
	ExecuteDelegateOnGameThread(OnListChangedDelegate);
	ExecuteDelegateOnGameThread(OnAssetCountChangedDelegate, TotalAssetCount, 0);

	AssetImportManager->RefreshGame();
}

void FWraithXViewModel::ImportSelectedAssets()
{
	if (bIsLoading || !AssetImportManager.IsValid() || SelectedItems.IsEmpty())
	{
		if (SelectedItems.IsEmpty())
		{
			ShowNotification(NSLOCTEXT("WraithXViewModel", "NoAssetsSelected", "No assets selected for import."));
		}
		return;
	}

	if (ImportPath.IsEmpty() || !ImportPath.StartsWith(TEXT("/Game/")))
	{
		ShowNotification(NSLOCTEXT("WraithXViewModel", "InvalidImportPath",
		                           "Please set a valid import path starting with /Game/."));
		return;
	}

	// TODO: Implement proper import flow
	// SetLoadingState(true); // Maybe a different "Importing" state?
	// UpdateLoadingProgress(0.0f); // Use a separate import progress if available

	ShowNotification(FText::Format(
		NSLOCTEXT("WraithXViewModel", "ImportStartedSelected", "Starting import for {0} selected assets to {1}..."),
		FText::AsNumber(SelectedItems.Num()), FText::FromString(ImportPath)));

	AssetImportManager->ImportSelection(ImportPath, SelectedItems, OptionalParams);

	// Need delegates from manager for import progress and completion to update UI state
}

void FWraithXViewModel::ImportAllAssets()
{
	if (bIsLoading || !AssetImportManager.IsValid() || FilteredItems.IsEmpty())
	{
		if (FilteredItems.IsEmpty())
		{
			ShowNotification(NSLOCTEXT("WraithXViewModel", "NoAssetsFiltered",
			                           "No assets match the current filter for import."));
		}
		return;
	}

	if (ImportPath.IsEmpty() || !ImportPath.StartsWith(TEXT("/Game/")))
	{
		ShowNotification(NSLOCTEXT("WraithXViewModel", "InvalidImportPath",
		                           "Please set a valid import path starting with /Game/."));
		return;
	}

	// TODO: Implement proper import flow
	// SetLoadingState(true);
	// UpdateLoadingProgress(0.0f);

	ShowNotification(FText::Format(
		NSLOCTEXT("WraithXViewModel", "ImportStartedAll", "Starting import for {0} filtered assets to {1}..."),
		FText::AsNumber(FilteredItems.Num()), FText::FromString(ImportPath)));

	AssetImportManager->ImportSelection(ImportPath, FilteredItems, OptionalParams);
	// Import the currently filtered list

	// Need delegates from manager for import progress and completion
}

void FWraithXViewModel::SetImportPath(const FString& Path)
{
	FString SanitizedPath = Path.TrimStartAndEnd();
	SanitizedPath.ReplaceInline(TEXT("\\"), TEXT("/"));

	if (!SanitizedPath.IsEmpty() && !SanitizedPath.StartsWith(TEXT("/Game/")))
	{
		ShowNotification(NSLOCTEXT("WraithXViewModel", "PathErrorHint",
		                           "Import path must start with /Game/ and be inside the Content directory."));
	}

	if (ImportPath != SanitizedPath)
	{
		ImportPath = SanitizedPath;
		ExecuteDelegateOnGameThread(OnImportPathChangedDelegate, ImportPath);
	}
}

void FWraithXViewModel::SetOptionalParams(const FString& Params)
{
	OptionalParams = Params;
}

void FWraithXViewModel::BrowseImportPath(TSharedPtr<SWindow> ParentWindow)
{
	if (bIsLoading) return;

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
			                           ? ParentWindow->GetNativeWindow()->GetOSWindowHandle()
			                           : nullptr;

		FString SelectedPath;
		const FString DefaultPath = FPaths::ProjectContentDir();

		if (DesktopPlatform->OpenDirectoryDialog(
			ParentWindowHandle,
			NSLOCTEXT("WraithXViewModel", "BrowseDialogTitle",
			          "Select Import Directory (Must be within Content Folder)").ToString(),
			DefaultPath,
			SelectedPath))
		{
			FString RelativePath = SelectedPath;
			if (FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectContentDir()))
			{
				RelativePath = FString(TEXT("/Game/")) + RelativePath;
				FPaths::NormalizeDirectoryName(RelativePath);
				if (!RelativePath.EndsWith(TEXT("/")))
				{
					RelativePath += TEXT("/");
				}
				SetImportPath(RelativePath);
			}
			else
			{
				ShowNotification(NSLOCTEXT("WraithXViewModel", "BrowseErrorHint",
				                           "Selected path must be within the project's Content directory."));
			}
		}
	}
	else
	{
		ShowNotification(NSLOCTEXT("WraithXViewModel", "DesktopPlatformError",
		                           "Cannot open directory browser. DesktopPlatform module not available."));
	}
}

void FWraithXViewModel::SetSortColumn(FName ColumnId)
{
	EWraithXSortColumn NewSortColumn = FNameToEWraithXSortColumn(ColumnId);
	if (NewSortColumn == EWraithXSortColumn::None) return;

	if (CurrentSortColumn != NewSortColumn)
	{
		CurrentSortColumn = NewSortColumn;
		CurrentSortMode = EColumnSortMode::Ascending;
	}
	else
	{
		CurrentSortMode = (CurrentSortMode == EColumnSortMode::Ascending)
			                  ? EColumnSortMode::Descending
			                  : EColumnSortMode::Ascending;
	}

	SortDataInternal(FilteredItems);
	ExecuteDelegateOnGameThread(OnListChangedDelegate);
}

void FWraithXViewModel::SetSelectedItems(const TArray<TSharedPtr<FCoDAsset>>& InSelectedItems)
{
	SelectedItems = InSelectedItems;
}

void FWraithXViewModel::RequestPreviewAsset(TSharedPtr<FCoDAsset> Item)
{
	if (Item.IsValid())
	{
		ExecuteDelegateOnGameThread(OnPreviewAssetDelegate, Item);
	}
}

void FWraithXViewModel::SetLoadingState(bool bNewLoadingState)
{
	if (bIsLoading != bNewLoadingState)
	{
		bIsLoading = bNewLoadingState;
		ExecuteDelegateOnGameThread(OnLoadingStateChangedDelegate, bIsLoading);

		if (!bIsLoading)
		{
			UpdateLoadingProgress(1.0f);
		}
	}
}

void FWraithXViewModel::UpdateLoadingProgress(float InProgress)
{
	CurrentLoadingProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	ExecuteDelegateOnGameThread(OnLoadingProgressChangedDelegate, CurrentLoadingProgress);
}

void FWraithXViewModel::HandleAssetInitCompleted()
{
	AsyncTask(ENamedThreads::GameThread, [Self = AsShared()]()
	{
		if (Self->AssetImportManager.IsValid())
		{
			Self->AllLoadedAssets = Self->AssetImportManager->GetLoadedAssets();
			Self->TotalAssetCount = Self->AllLoadedAssets.Num();
		}
		else
		{
			Self->AllLoadedAssets.Empty();
			Self->TotalAssetCount = 0;
		}

		Self->ApplyFilteringAndSortingAsync();

		Self->SetLoadingState(false);
	});
}

void FWraithXViewModel::HandleAssetLoadingProgress(float InProgress)
{
	UpdateLoadingProgress(InProgress);
}

void FWraithXViewModel::ApplyFilteringAndSortingAsync()
{
	CancelCurrentAsyncTask();

	FString SearchText = CurrentSearchText;
	TArray<TSharedPtr<FCoDAsset>> AssetsToProcess = AllLoadedAssets;
	EWraithXSortColumn SortColumn = CurrentSortColumn;
	EColumnSortMode::Type SortMode = CurrentSortMode;
	int32 CurrentTotalCount = TotalAssetCount;

	CurrentAsyncTask = Async(EAsyncExecution::ThreadPool,
	                         [this, AssetsToProcess, SearchText, SortColumn, SortMode, CurrentTotalCount
	                         ]() mutable -> TArray<TSharedPtr<FCoDAsset>>
	                         {
		                         TArray<TSharedPtr<FCoDAsset>> TempFilteredItems = FilterAssets_WorkerThread(
			                         AssetsToProcess, SearchText);

		                         SortDataInternal(TempFilteredItems);

		                         return TempFilteredItems;
	                         }).Then([this, CurrentTotalCount](TFuture<TArray<TSharedPtr<FCoDAsset>>> ResultFuture)
	{
		if (!this) return;

		TArray<TSharedPtr<FCoDAsset>> NewFilteredItems = ResultFuture.Get();

		FinalizeUpdate_GameThread(MoveTemp(NewFilteredItems), CurrentTotalCount);
	});
}

void FWraithXViewModel::CancelCurrentAsyncTask()
{
	if (CurrentAsyncTask.IsValid())
	{
		CurrentAsyncTask.Reset();
	}
}

TArray<TSharedPtr<FCoDAsset>> FWraithXViewModel::FilterAssets_WorkerThread(
	const TArray<TSharedPtr<FCoDAsset>>& AssetsToFilter, const FString& SearchText)
{
	TArray<TSharedPtr<FCoDAsset>> Result;
	Result.Reserve(AssetsToFilter.Num());

	if (SearchText.IsEmpty())
	{
		Result = AssetsToFilter;
	}
	else
	{
		TArray<FString> SearchTokens;
		SearchText.ParseIntoArray(SearchTokens, TEXT(" "), true);

		for (const auto& Item : AssetsToFilter)
		{
			if (Item.IsValid() && MatchSearchCondition(Item, SearchTokens))
			{
				Result.Add(Item);
			}
		}
	}
	return Result;
}

void FWraithXViewModel::SortDataInternal(TArray<TSharedPtr<FCoDAsset>>& ItemsToSort)
{
	if (CurrentSortColumn == EWraithXSortColumn::None || ItemsToSort.Num() < 2)
	{
		return;
	}

	bool bAscending = (CurrentSortMode == EColumnSortMode::Ascending);

	switch (CurrentSortColumn)
	{
	case EWraithXSortColumn::AssetName:
		ItemsToSort.Sort([bAscending](const TSharedPtr<FCoDAsset>& A, const TSharedPtr<FCoDAsset>& B)
		{
			if (!A.IsValid()) return false;
			if (!B.IsValid()) return true;
			int32 CompareResult = A->AssetName.Compare(B->AssetName/*, ESearchCase::IgnoreCase*/);
			return bAscending ? (CompareResult < 0) : (CompareResult > 0);
		});
		break;

	case EWraithXSortColumn::Status:
		ItemsToSort.Sort([bAscending](const TSharedPtr<FCoDAsset>& A, const TSharedPtr<FCoDAsset>& B)
		{
			if (!A.IsValid()) return false;
			if (!B.IsValid()) return true;
			int32 CompareResult = A->GetAssetStatusText().CompareTo(
				B->GetAssetStatusText()/*, ETextComparisonLevel::IgnoreCase*/);
			return bAscending ? (CompareResult < 0) : (CompareResult > 0);
		});
		break;

	case EWraithXSortColumn::Type:
		ItemsToSort.Sort([bAscending](const TSharedPtr<FCoDAsset>& A, const TSharedPtr<FCoDAsset>& B)
		{
			if (!A.IsValid()) return false;
			if (!B.IsValid()) return true;
			int32 CompareResult = A->GetAssetTypeText().CompareTo(
				B->GetAssetTypeText()/*, ETextComparisonLevel::IgnoreCase*/);
			return bAscending ? (CompareResult < 0) : (CompareResult > 0);
		});
		break;

	case EWraithXSortColumn::Size:
		ItemsToSort.Sort([bAscending](const TSharedPtr<FCoDAsset>& A, const TSharedPtr<FCoDAsset>& B)
		{
			if (!A.IsValid()) return false;
			if (!B.IsValid()) return true;
			if (A->AssetSize == B->AssetSize) return false;
			return bAscending ? (A->AssetSize < B->AssetSize) : (A->AssetSize > B->AssetSize);
		});
		break;

	default:
		break;
	}
}

void FWraithXViewModel::FinalizeUpdate_GameThread(TArray<TSharedPtr<FCoDAsset>> NewFilteredItems, int32 NewTotalCount)
{
	check(IsInGameThread());

	FilteredItems = MoveTemp(NewFilteredItems);
	TotalAssetCount = NewTotalCount;

	OnListChangedDelegate.ExecuteIfBound();
	OnAssetCountChangedDelegate.ExecuteIfBound(TotalAssetCount, FilteredItems.Num());
}

bool FWraithXViewModel::MatchSearchCondition(const TSharedPtr<FCoDAsset>& Item, const TArray<FString>& SearchTokens)
{
	if (!Item.IsValid()) return false;

	if (SearchTokens.IsEmpty()) return true;

	for (const FString& Token : SearchTokens)
	{
		if (!Item->AssetName.Contains(Token/*, ESearchCase::IgnoreCase*/))
		{
			return false;
		}
	}
	return true;
}

void FWraithXViewModel::ShowNotification(const FText& Message)
{
	ExecuteDelegateOnGameThread(OnShowNotificationDelegate, Message);
}
