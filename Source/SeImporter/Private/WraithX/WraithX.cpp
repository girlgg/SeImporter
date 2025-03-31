#include "WraithX/WraithX.h"

#include "WraithX/GameProcess.h"

void FWraithX::InitializeGame()
{
	if (ProcessInstance.IsValid())
	{
		ProcessInstance.Reset();
	}
	ProcessInstance = MakeUnique<FGameProcess>();
	FAssetNameDB::Get().GetTaskCompletedHandle().AddRaw(this, &FWraithX::OnAssetInitCompletedCall);
}

void FWraithX::OnAssetInitCompletedCall()
{
	OnOnAssetInitCompletedDelegate.Broadcast();
	OnOnAssetInitCompletedDelegate.Clear();
}
