#pragma once

#include "CoreMinimal.h"
#include "MapImporter/CastMapImporter.h"
#include "Modules/ModuleManager.h"
#include "Internationalization/Text.h"

enum class ECastTextureImportType : uint8;

class IWTOUE_API FIWToUEModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TUniquePtr<FCastMapImporter> MapImporter{MakeUnique<FCastMapImporter>()};
};
