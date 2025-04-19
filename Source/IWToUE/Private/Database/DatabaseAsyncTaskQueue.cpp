#include "Database/DatabaseAsyncTaskQueue.h"

#include "SeLogChannels.h"

FDatabaseAsyncTaskQueue::FDatabaseAsyncTaskQueue()
{
}

FDatabaseAsyncTaskQueue::~FDatabaseAsyncTaskQueue()
{
	TaskStop();
	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
	}
}

void FDatabaseAsyncTaskQueue::EnqueueTask(TFunction<void()> Task, TFunction<void()> CompletionCallback)
{
	if (!Task) return;
	FScopeLock Lock(&QueueLock);
	TaskQueue.Enqueue({MoveTemp(Task), MoveTemp(CompletionCallback)});
}

void FDatabaseAsyncTaskQueue::TaskStart()
{
	if (!Thread)
	{
		bShouldRun = true;
		Thread = FRunnableThread::Create(this, TEXT("DatabaseAsyncTaskQueueThread"));
		UE_LOG(LogITUDatabase, Log, TEXT("Started Async Database Task Queue Thread."));
	}
}

void FDatabaseAsyncTaskQueue::TaskStop()
{
	bShouldRun = false;
}

bool FDatabaseAsyncTaskQueue::Init()
{
	return FRunnable::Init();
}

uint32 FDatabaseAsyncTaskQueue::Run()
{
	UE_LOG(LogTemp, Log, TEXT("Async Database Task Queue Thread Running."));
	while (bShouldRun)
	{
		FQueuedTask CurrentTask;
		bool bDequeued = false;
		{
			FScopeLock Lock(&QueueLock);
			bDequeued = TaskQueue.Dequeue(CurrentTask);
		}

		if (bDequeued)
		{
			if (CurrentTask.Task)
			{
				CurrentTask.Task();
			}

			if (CurrentTask.CompletionCallback)
			{
				AsyncTask(ENamedThreads::GameThread, MoveTemp(CurrentTask.CompletionCallback));
			}

			bool bIsEmptyNow;
			{
				FScopeLock Lock(&QueueLock);
				bIsEmptyNow = TaskQueue.IsEmpty();
			}
			if (bIsEmptyNow)
			{
				AsyncTask(ENamedThreads::GameThread, [this]()
				{
					if (this) OnQueueEmptyDelegate.ExecuteIfBound();
				});
			}
		}
		else
		{
			FPlatformProcess::Sleep(0.05f);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Async Database Task Queue Thread Exiting."));
	return 0;
}

void FDatabaseAsyncTaskQueue::Stop()
{
	TaskStop();
}
