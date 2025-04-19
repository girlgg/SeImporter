#pragma once

#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <string>
#include "Windows/HideWindowsPlatformTypes.h"

#include "CoreMinimal.h"

class IMemoryReader
{
public:
	virtual ~IMemoryReader() = default;
	virtual bool IsValid() const = 0;
	virtual HANDLE GetProcessHandle() const = 0;

	template <typename T>
	bool ReadMemory(uint64 Address, T& OutResult, bool bIsLocal = false);

	template <typename T>
	bool ReadArray(uint64 Address, TArray<T>& OutArray, uint64 Length);

	virtual bool ReadString(uint64 Address, FString& OutString, int MaxLength = 20480) = 0;

protected:
	virtual bool ReadMemoryImpl(uint64 Address, void* OutResult, size_t Size) = 0;
	virtual bool ReadArrayImpl(uint64 Address, void* OutArrayData, uint64 Length, size_t ElementSize) = 0;
};

#include "IMemoryReader.inl"
