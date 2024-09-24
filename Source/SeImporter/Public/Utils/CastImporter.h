#pragma once

#include "Widgets/CastImportUI.h"
#include "CastManager/CastManager.h"
#include "CastManager/CastModel.h"
#include "CastManager/CastScene.h"

class FCastFileTexture;
struct FCastScene;
struct FCastSceneInfo;
class FCastNode;

namespace CastScene
{
	class FImportSkeletalMeshArgs
	{
	public:
		FImportSkeletalMeshArgs()
			: InParent(nullptr)
			  , Name(NAME_None)
			  , Flags(RF_NoFlags)
			  , LodIndex(0)
			  , OutData(nullptr)
			  , bCreateRenderData(true)
			  , OrderedMaterialNames(nullptr)
			  , ImportMaterialOriginalNameData(nullptr)
			  , bMapMorphTargetToTimeZero(false)
		{
		}

		UObject* InParent;
		FName Name;
		EObjectFlags Flags;
		int32 LodIndex;
		FSkeletalMeshImportData* OutData;
		bool bCreateRenderData;
		TArray<FName>* OrderedMaterialNames;

		TArray<FName>* ImportMaterialOriginalNameData;
		bool bMapMorphTargetToTimeZero;
	};
}

struct FCastImportOptions
{
	UPhysicsAsset* PhysicsAsset{nullptr};
	USkeleton* Skeleton{nullptr};

	FString GlobalMaterialPath;
	FString TextureFormat;

	bool bImportMaterial;
	bool bUseGlobalMaterialsPath;
	bool bImportAsSkeletal;
	bool bImportMesh;
	bool bImportAnimations;
	bool bImportAnimationNotify{true};
	ECastAnimImportType AnimImportType;
	ECastEngineType EngineType;
};

struct FCastMaterial
{
	FCastMaterialInfo* CastMaterial{nullptr};
	UMaterialInterface* Material{nullptr};

	FString GetName() const
	{
		return CastMaterial ? CastMaterial->GetName() : Material ? Material->GetName() : TEXT("None");
	}
};

class FCastImporter
{
public:
	FCastImporter();
	~FCastImporter();
	void CleanUp();

	void ReleaseScene();

	int32 GetImportType(const FString& InFilename);
	bool OpenFile(FString Filename);
	bool GetSceneInfo(FString Filename);
	bool ImportFile(FString Filename);
	bool ImportFromFile(FString Filename);
	void AnalysisMaterial(FString ParentPath, FString MaterialPath, FString TexturePath,
	                      FString TextureFormat);
	bool AnalysisTexture(FCastTextureInfo& Texture, FString ParentPath, FString TextureLineText, FString TexturePath,
	                     const FString& ImageFormat);
	bool ImportTexture(FCastTextureInfo& Texture, const FString& FilePath, const FString& ParentPath, bool bSRGB);
	UMaterialInterface* CreateMaterialInstance(const FCastMaterialInfo& Material, const UObject* ParentPackage);

	void ClearAllCaches();

	static FCastImporter* GetInstance(bool bDoNotCreate = false);
	static void DeleteInstance();

	static FString MakeName(const FString& Name);

	FCastImportOptions* GetImportOptions() const;
	FCastImportOptions* GetImportOptions(UCastImportUI* ImportUI, bool bShowOptionDialog,
	                                     bool bIsAutomated, const FString& FullPath,
	                                     bool& OutOperationCanceled,
	                                     bool& OutImportAll, const FString& InFilename);

	USkeletalMesh* ImportSkeletalMesh(CastScene::FImportSkeletalMeshArgs& ImportSkeletalMeshArgs);
	UStaticMesh* ImportStaticMesh(UObject* InParent, const FName InName, EObjectFlags Flags);
	UAnimSequence* ImportAnim(UObject* InParent, USkeleton* Skeleton);

	void BulidStaticMeshFromModel(UObject* ParentPackage, FCastModelInfo& Model, UStaticMesh* StaticMesh);

	static UObject* CreateAssetOfClass(UClass* AssetClass, FString ParentPackageName, FString ObjectName,
	                                   bool bAllowReplace = false);

	template <class T>
	static T* CreateAsset(FString ParentPackageName, FString ObjectName, bool bAllowReplace = false)
	{
		return (T*)CreateAssetOfClass(T::StaticClass(), ParentPackageName, ObjectName, bAllowReplace);
	}

public:
	FCastImportOptions* ImportOptions;
	FCastSceneInfo SceneInfo;
	FString FileBasePath;

	FMD5Hash Md5Hash;

protected:
	enum IMPORTPHASE
	{
		NOTSTARTED,
		FILEOPENED,
		IMPORTED,
		FIXEDANDCONVERTED,
	};

	static TSharedPtr<FCastImporter> StaticInstance;
	IMPORTPHASE CurPhase;

	bool ImportAnimation(USkeleton* Skeleton, UAnimSequence* DestSeq, FCastAnimationInfo& Animation);
	template <typename T>
	void InterpolateAnimationKeyframes(TArray<T>& BoneCurve, const TArray<uint32>& KeyFrameBuffer,
	                                   const TArray<FVariant>& KeyValueBuffer);
	template <typename T>
	T LerpValue(const uint32& LastKey,
	            const uint32& CurrentKey,
	            const uint32& EndKey,
	            const T& LastValue,
	            const T& EndValue);

private:
	TArray<TWeakObjectPtr<UObject>> CreatedObjects;

	FCastManager* CastManager{nullptr};
	TArray<FCastMaterialInfo> ImportedMaterials;
};

template <typename T>
FORCEINLINE void FCastImporter::InterpolateAnimationKeyframes(TArray<T>& BoneCurve,
                                                              const TArray<uint32>& KeyFrameBuffer,
                                                              const TArray<FVariant>& KeyValueBuffer)
{
	uint32 CurrentFrame = 0;
	uint32 LastKeyFrame = 0;
	T LastKeyValue = T();
	for (int32 i = 0; i < KeyFrameBuffer.Num(); ++i)
	{
		if (i == 0)
		{
			BoneCurve.Add(KeyValueBuffer[i].GetValue<T>());
		}
		else
		{
			while (CurrentFrame <= KeyFrameBuffer[i])
			{
				while (!BoneCurve.IsValidIndex(CurrentFrame)) BoneCurve.AddZeroed();
				BoneCurve[CurrentFrame] = LerpValue(LastKeyFrame,
				                                    CurrentFrame,
				                                    KeyFrameBuffer[i],
				                                    LastKeyValue,
				                                    KeyValueBuffer[i].GetValue<T>());
				++CurrentFrame;
			}
		}
		LastKeyFrame = KeyFrameBuffer[i];
		LastKeyValue = KeyValueBuffer[i].GetValue<T>();
	}
}

template <typename T>
FORCEINLINE T FCastImporter::LerpValue(const uint32& LastKey, const uint32& CurrentKey, const uint32& EndKey,
                                       const T& LastValue,
                                       const T& EndValue)
{
	return FMath::Lerp(LastValue, EndValue, (float)(CurrentKey - LastKey) / (float)(EndKey - LastKey));
}
