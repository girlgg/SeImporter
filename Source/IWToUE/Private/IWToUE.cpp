#include "IWToUE.h"

#include "MapImporter/CastMapImporter.h"

#define LOCTEXT_NAMESPACE "FIWToUEModule"

void FIWToUEModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		MapImporter->RegisterMenus();
	}));
}

void FIWToUEModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FIWToUEModule, IWToUE)
