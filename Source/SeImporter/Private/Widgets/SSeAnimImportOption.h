// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

enum class EImportOptionDlgResponse : uint8
{
	Import,
	ImportAll,
	Cancel
};

class SSeAnimImportOption final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSeAnimImportOption)
			: _WidgetWindow()
		{
		}

		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_END_ARGS()

	SSeAnimImportOption()
		: UserDlgResponse(EImportOptionDlgResponse::Cancel)
	{
	}

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	/** A property view to edit advanced options */
	TSharedPtr<IDetailsView> PropertyView;

	// UPROPERTY(Category = MapsAndSets, EditAnywhere)
	mutable class USeAnimOptions* Stun{nullptr};

	bool bShouldImportAll{false};
	/** If we should import */
	bool ShouldImport();

	/** If the current settings should be applied to all items being imported */
	bool ShouldImportAll();


	FReply OnImportAll();
	/** Called when 'Apply' button is pressed */
	FReply OnImport();

	FReply OnCancel();

private:
	EImportOptionDlgResponse UserDlgResponse;
	FReply HandleImport();

	/** Window that owns us */
	TWeakPtr<SWindow> WidgetWindow;
};
