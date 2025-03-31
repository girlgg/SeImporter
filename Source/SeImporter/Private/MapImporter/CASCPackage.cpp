#include "MapImporter/CASCPackage.h"

void FCASCPackage::LoadFiles(const FString& GamePath)
{
	FString BuildInfoPath = FPaths::Combine(GamePath, TEXT(".build.info"));
	if (FPaths::FileExists(BuildInfoPath))
	{
		/*storage = MakeUnique<CascStorage>(GamePath);

		const TMap<FString, CascStorage::CascFile>& FileEntries = storage->FileSystemHandler.FileEntries;

		int32 FileIndex = 0;
		for (const auto& Entry : FileEntries)
		{
			const CascStorage::CascFile& CascFile = Entry.Value;

			// 检查文件是否存在且扩展名为 .xsub
			if (!CascFile.Exists || !FPaths::GetExtension(CascFile.Name).EndsWith(TEXT("xsub")))
			{
				continue;
			}

			TUniquePtr<FArchive> Reader = CascFile.OpenFile(CascFile.Name);
			if (Reader)
			{
				ReadCascXSub(*Reader, FileIndex, Assets);

				files.Add(CascFile.Name);
				FileIndex++;
			}
		}*/
	}
}
