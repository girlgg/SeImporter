#pragma once

#include "CoreMinimal.h"
#include "SeLogChannels.h"
#include "Interface/IMemoryReader.h"

class FWindowsMemoryReader final : public IMemoryReader
{
public:
	explicit FWindowsMemoryReader(DWORD ProcessId);
	virtual ~FWindowsMemoryReader() override;

	virtual bool IsValid() const override { return ProcessHandle != nullptr; }
	virtual HANDLE GetProcessHandle() const override { return ProcessHandle; }

	virtual bool ReadString(uint64 Address, FString& OutString, int MaxLength = 20480) override;

protected:
	virtual bool ReadMemoryImpl(uint64 Address, void* OutResult, size_t Size) override;
	virtual bool ReadArrayImpl(uint64 Address, void* OutArrayData, uint64 Length, size_t ElementSize) override;

	bool ReadNullTerminatedString(uint64 Address, TArray<ANSICHAR>& OutBuffer, int32 MaxLength);

private:
	HANDLE ProcessHandle = nullptr;
	FCriticalSection ReadLock;

	static HANDLE OpenTargetProcess(DWORD ProcessId);
};

