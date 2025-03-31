// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MapImporter/CastMapImporter.h"
#include "Modules/ModuleManager.h"

enum class ECastTextureImportType : uint8;

class FSeImporterModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TUniquePtr<FCastMapImporter> MapImporter{MakeUnique<FCastMapImporter>()};
};
