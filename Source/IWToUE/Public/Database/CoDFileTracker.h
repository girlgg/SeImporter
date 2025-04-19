#pragma once

#include "CoreMinimal.h"
#include "Interface/IFileTracker.h"


class IXSubInfoRepository;
class IAssetNameRepository;
class IGameRepository;
class IFileMetaRepository;
struct FXSubPackageCacheObject;

class IWTOUE_API FCoDFileTracker : public IFileTracker, public FRunnable, public TSharedFromThis<FCoDFileTracker>
{
public:
	FCoDFileTracker(
		const TSharedPtr<IFileMetaRepository>& InFileMetaRepo,
		const TSharedPtr<IGameRepository>& InGameRepo,
		const TSharedPtr<IAssetNameRepository>& InAssetNameRepo,
		const TSharedPtr<IXSubInfoRepository>& InXSubInfoRepo
	);
	virtual ~FCoDFileTracker() override;

	//~ Begin IFileTracker interface
	virtual void Initialize(uint64 GameHash, const FString& GamePath) override;
	virtual void StartTrackingAsync() override;
	virtual bool IsTrackingComplete() const override { return bIsComplete; }
	//~ End of IFileTracker interface

	//~ Begin FRunnable interface
	virtual bool Init() override { return true; }
	virtual uint32 Run() override;
	virtual void Stop() override;
	//~ End of FRunnable interface

	virtual void Exit() override
	{
	}

private:
	void ProcessFiles();
	TArray<FString> FindWniFilesToTrack() const;
	TArray<FString> FindXSubFilesToTrack() const;
	FString GetRelativePath(const FString& FullPath, const FString& BaseDir) const;
	uint64 ComputeFileContentHash(const FString& FilePath, int64 ComputeSize = 1024 * 1024) const;
	int64 GetFileLastModifiedTime(const FString& FilePath) const;

	void ParseWniFile(const FString& FilePath, TMap<uint64, FString>& Items);
	void ParseXSubFile(const FString& FilePath, TMap<uint64, FXSubPackageCacheObject>& Items, int64 FileId);

	bool CheckIfFileNeedsUpdate(uint64 GameHash, const FString& RelativePath, uint64 NewContentHash,
	                            int64 NewLastModifiedTime, int64& OutFileId);

	TSharedPtr<IFileMetaRepository> FileMetaRepo;
	TSharedPtr<IGameRepository> GameRepo;
	TSharedPtr<IAssetNameRepository> AssetNameRepo;
	TSharedPtr<IXSubInfoRepository> XSubInfoRepo;

	FRunnableThread* Thread = nullptr;
	FThreadSafeBool bShouldRun = false;
	FThreadSafeBool bIsComplete = true;
	uint64 CurrentGameHash = 0;
	FString CurrentGamePath;
	FString PluginBasePath;
};
