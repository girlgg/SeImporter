#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UUserMeshOptions;
class SeModel;

enum class EPSAImportOptionDlgResponse : uint8
{
	Import,
	ImportAll,
	Cancel
};

class SEIMPORTER_API SSeModelImportOptions final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSeModelImportOptions)
			: _MeshHeader(nullptr), _WidgetWindow()
		{
		}

		SLATE_ARGUMENT(SeModel*, MeshHeader)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	static TSharedPtr<SHorizontalBox> AddNewHeaderProperty(FText InKey, FText InValue);
	static TSharedPtr<SBorder> CreateMapHeader(SeModel* MeshHeader);
	/** A property view to edit advanced options */
	TSharedPtr<class IDetailsView> PropertyView;

	UPROPERTY(Category = MapsAndSets, EditAnywhere)
	mutable UUserMeshOptions* Options{nullptr};

	bool bShouldImportAll{false};
	/** If we should import */
	bool ShouldImport();

	/** If the current settings should be applied to all items being imported */
	bool ShouldImportAll();
	bool ShouldCancel();

	FReply OnImportAll();
	/** Called when 'Apply' button is pressed */
	FReply OnImport();

	FReply OnCancel();

private:
	SeModel* MeshHeader{nullptr};
	EPSAImportOptionDlgResponse UserDlgResponse{EPSAImportOptionDlgResponse::Cancel};
	FReply HandleImport();

	/** Window that owns us */
	TWeakPtr<SWindow> WidgetWindow;
};
