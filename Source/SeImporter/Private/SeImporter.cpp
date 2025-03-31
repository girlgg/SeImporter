#include "SeImporter.h"

#include "MapImporter/CastMapImporter.h"

#define LOCTEXT_NAMESPACE "FSeImporterModule"

void FSeImporterModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		MapImporter->RegisterMenus();
	}));
}

void FSeImporterModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSeImporterModule, SeImporter)
