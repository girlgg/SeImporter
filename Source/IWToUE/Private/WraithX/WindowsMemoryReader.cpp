#include "WraithX/WindowsMemoryReader.h"

FWindowsMemoryReader::FWindowsMemoryReader(DWORD ProcessId)
{
	ProcessHandle = OpenTargetProcess(ProcessId);
}

FWindowsMemoryReader::~FWindowsMemoryReader()
{
	if (ProcessHandle)
	{
		CloseHandle(ProcessHandle);
		ProcessHandle = nullptr;
	}
}

bool FWindowsMemoryReader::ReadString(uint64 Address, FString& OutString, int MaxLength)
{
	OutString.Empty();
	if (!IsValid() || Address == 0) return false;

	TArray<ANSICHAR> StringBuffer;
	if (!ReadNullTerminatedString(Address, StringBuffer, MaxLength))
	{
		OutString.Empty();
		return false;
	}

	if (StringBuffer.Num() > 0)
	{
		StringBuffer.Add('\0');
		OutString = FString(StringBuffer.GetData());
	}
	else
	{
		OutString = TEXT("");
	}

	return true;
}

bool FWindowsMemoryReader::ReadMemoryImpl(uint64 Address, void* OutResult, size_t Size)
{
	if (!IsValid() || Address == 0) return false;

	FScopeLock Lock(&ReadLock);
	SIZE_T BytesRead = 0;
	if (!::ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(Address), OutResult, Size, &BytesRead))
	{
		UE_LOG(LogITUMemoryReader, Verbose, TEXT("Failed to read memory from address: 0x%llX. Error: %d"), Address,
		       GetLastError());
		return false;
	}
	return BytesRead == Size;
}

bool FWindowsMemoryReader::ReadArrayImpl(uint64 Address, void* OutArrayData, uint64 Length, size_t ElementSize)
{
	if (!IsValid() || Address == 0)
	{
		return false;
	}

	FScopeLock Lock(&ReadLock);
	uint64 TotalBytesToRead = Length * ElementSize;
	SIZE_T BytesRead = 0;

	if (!::ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(Address), OutArrayData, TotalBytesToRead,
	                         &BytesRead))
	{
		UE_LOG(LogITUMemoryReader, Error, TEXT("Failed to read memory array from address: 0x%llX. Error: %d"), Address,
		       GetLastError());
		return false;
	}

	if (BytesRead != TotalBytesToRead)
	{
		UE_LOG(LogITUMemoryReader, Warning,
		       TEXT("Partial read occurred for array at 0x%llX. Expected %llu bytes, got %llu."),
		       Address, TotalBytesToRead, BytesRead);
		return false;
	}
	return true;
}

bool FWindowsMemoryReader::ReadNullTerminatedString(uint64 Address, TArray<ANSICHAR>& OutBuffer, int32 MaxLength)
{
	OutBuffer.Empty();
	OutBuffer.Reserve(FMath::Min(MaxLength, 128));

	uint64 CurrentAddress = Address;
	bool bSuccess = true;

	for (int32 CharIdx = 0; CharIdx < MaxLength; ++CharIdx)
	{
		ANSICHAR Char;
		if (!ReadMemoryImpl(CurrentAddress, &Char, sizeof(ANSICHAR)))
		{
			bSuccess = false;
			break;
		}

		if (Char == '\0')
		{
			break;
		}

		OutBuffer.Add(Char);
		CurrentAddress++;
	}

	if (!bSuccess && OutBuffer.Num() == 0)
	{
		return false;
	}

	if (OutBuffer.Num() >= MaxLength && OutBuffer.Last() != '\0')
	{
		UE_LOG(LogITUMemoryReader, Warning, TEXT("String at 0x%llX exceeded MaxLength (%d)"), Address, MaxLength);
	}

	return true;
}

HANDLE FWindowsMemoryReader::OpenTargetProcess(DWORD ProcessId)
{
	if (ProcessId == 0) return nullptr;
	return OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, ProcessId);
}
