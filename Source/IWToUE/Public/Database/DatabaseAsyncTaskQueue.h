#pragma once
#include "Interface/IAsyncTaskQueue.h"

class FDatabaseAsyncTaskQueue : public IAsyncTaskQueue, public FRunnable
{
public:
	FDatabaseAsyncTaskQueue();
	virtual ~FDatabaseAsyncTaskQueue() override;

	//~ Begin IAsyncTaskQueue interface
	virtual void EnqueueTask(TFunction<void()> Task, TFunction<void()> CompletionCallback = nullptr) override;
	virtual void TaskStart() override;
	virtual void TaskStop() override;
	virtual FOnTaskQueueEmptyDelegate& GetOnQueueEmptyDelegate() override { return OnQueueEmptyDelegate; }
	//~ End of IAsyncTaskQueue interface

	//~ Begin FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	//~ End of FRunnable interface
private:
	struct FQueuedTask
	{
		TFunction<void()> Task;
		TFunction<void()> CompletionCallback;
	};

	FRunnableThread* Thread = nullptr;
	FThreadSafeBool bShouldRun = false;
	FCriticalSection QueueLock;
	TQueue<FQueuedTask> TaskQueue;
	FOnTaskQueueEmptyDelegate OnQueueEmptyDelegate;
};
