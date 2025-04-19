#pragma once

DECLARE_DELEGATE(FOnTaskQueueEmptyDelegate);

class IAsyncTaskQueue
{
public:
	virtual ~IAsyncTaskQueue() = default;
	virtual void EnqueueTask(TFunction<void()> Task, TFunction<void()> CompletionCallback = nullptr) = 0;
	virtual void TaskStart() = 0;
	virtual void TaskStop() = 0;
	virtual FOnTaskQueueEmptyDelegate& GetOnQueueEmptyDelegate() = 0;
};
