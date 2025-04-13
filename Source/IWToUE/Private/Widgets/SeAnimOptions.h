// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SeAnimOptions.generated.h"

/**
 * 
 */
UCLASS()
class USeAnimOptions : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Import Settings")
	TSoftObjectPtr<USkeleton> Skeleton;

	bool bInitialized{false};
};
