#include "Factories/SeAnimAssetFactory.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Serialization/LargeMemoryReader.h"
#include "Structures/SeAnim.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/SeAnimOptions.h"
#include "Widgets/SSeAnimImportOption.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SeAnimAssetFactory)
#define LOCTEXT_NAMESPACE "C2AnimAssetFactory"

USeAnimAssetFactory::USeAnimAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Formats.Add(TEXT("seanim;seanim Animation File"));
	SupportedClass = UAnimSequence::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
	SettingsImporter = CreateDefaultSubobject<USeAnimOptions>(TEXT("Anim Options"));
}

UObject* USeAnimAssetFactory::FactoryCreateFile
(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	const FString& Filename,
	const TCHAR* Parms,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled
)
{
	FScopedSlowTask SlowTask(5, NSLOCTEXT("SeImporterFactory", "BeginReadSeFile", "Opening SeAnim file."), true);
	if (Warn->GetScopeStack().Num() == 0)
	{
		SlowTask.MakeDialog(true);
	}
	SlowTask.EnterProgressFrame(0);

	if (SettingsImporter->bInitialized == false)
	{
		TSharedPtr<SSeAnimImportOption> ImportOptionsWindow;
		TSharedPtr<SWindow> ParentWindow;
		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			const IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		const TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(FText::FromString(TEXT("SEAnim Import Options")))
			.SizingRule(ESizingRule::Autosized);
		Window->SetContent
		(
			SAssignNew(ImportOptionsWindow, SSeAnimImportOption)
			.WidgetWindow(Window)
		);
		SettingsImporter = ImportOptionsWindow.Get()->Stun;
		FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
		bImport = ImportOptionsWindow.Get()->ShouldImport();
		bImportAll = ImportOptionsWindow.Get()->ShouldImportAll();
		SettingsImporter->bInitialized = true;
	}

	UAnimSequence* AnimSequence = NewObject<UAnimSequence>(InParent, UAnimSequence::StaticClass(), InName, Flags);

	constexpr bool bShouldTransact = false;
	USkeleton* Skeleton = SettingsImporter->Skeleton.LoadSynchronous();
	AnimSequence->SetSkeleton(Skeleton);

	IAnimationDataController& Controller = AnimSequence->GetController();
	Controller.OpenBracket(LOCTEXT("ImportAnimation_Bracket", "Importing Animation"));
	Controller.InitializeModel();

	AnimSequence->ResetAnimation();
	Bones = Skeleton->GetReferenceSkeleton().GetRawRefBoneInfo();
	if (TArray64<uint8> FileDataOld; FFileHelper::LoadFileToArray(FileDataOld, *Filename))
	{
		FLargeMemoryReader Reader(FileDataOld.GetData(), FileDataOld.Num());
		FSeAnim* Anim = new FSeAnim();
		Anim->ParseAnim(Reader);

		Controller.SetFrameRate(FFrameRate(Anim->Header.FrameRate, 1), bShouldTransact);
		Controller.SetNumberOfFrames(FFrameNumber(int(Anim->Header.FrameCountBuffer)), bShouldTransact);
		UE_LOG(LogTemp, Warning, TEXT("This animation '%s' is of type %s"), *Filename,
		       *UEnum::GetValueAsString(Anim->Header.AnimType));
		for (int32 BoneTreeIndex = 0; BoneTreeIndex < Bones.Num(); BoneTreeIndex++)
		{
			const FName BoneTreeName = Skeleton->GetReferenceSkeleton().GetBoneName(BoneTreeIndex);
			Controller.AddBoneCurve(BoneTreeName, bShouldTransact);
		}

		for (int32 BoneIndex = 0; BoneIndex < Anim->BonesInfos.Num(); BoneIndex++)
		{
			FBoneInfo& KeyFrameBone = Anim->BonesInfos[BoneIndex];
			FMeshBoneInfo BoneMesh = GetBone(KeyFrameBone.Name);
			if (BoneMesh.Name == "None") { continue; }
			FName NewCurveName(KeyFrameBone.Name);


			TArray<FVector3f> PositionalKeys;
			TArray<FQuat4f> RotationalKeys;
			TArray<FVector3f> ScalingKeys;

			// 设置位置关键帧
			uint32 CurrentFrame = 0;
			for (int32 i = 0; i < KeyFrameBone.BonePositions.Num(); ++i, ++CurrentFrame)
			{
				TWraithAnimFrame<FVector3f>& BonePosAnimFrame = KeyFrameBone.BonePositions[i];
				BonePosAnimFrame.Value[1] *= -1;
				// 手动插值
				if (i == 0 && BonePosAnimFrame.Frame > 0)
				{
					while (CurrentFrame < BonePosAnimFrame.Frame)
					{
						PositionalKeys.Add(BonePosAnimFrame.Value);
						++CurrentFrame;
					}
				}
				else
				{
					while (CurrentFrame < BonePosAnimFrame.Frame)
					{
						TWraithAnimFrame<FVector3f> LastBonePosAnimFrame = KeyFrameBone.BonePositions[i - 1];
						PositionalKeys.Add(
							FMath::Lerp(LastBonePosAnimFrame.Value, BonePosAnimFrame.Value,
							            static_cast<float>(CurrentFrame - LastBonePosAnimFrame.Frame) /
							            static_cast<float>(BonePosAnimFrame.Frame - LastBonePosAnimFrame.Frame)));
						++CurrentFrame;
					}
				}
				PositionalKeys.Add(BonePosAnimFrame.Value);
			}

			// 设置旋转关键帧
			FQuat4f LastRotator;
			CurrentFrame = 0;
			for (int32 i = 0; i < KeyFrameBone.BoneRotations.Num(); ++i, ++CurrentFrame)
			{
				TWraithAnimFrame<FQuat4f> BoneRotationKeyFrame = KeyFrameBone.BoneRotations[i];
				// Unreal uses other axis type than COD engine
				FRotator3f LocalRotator = BoneRotationKeyFrame.Value.Rotator();
				LocalRotator.Yaw *= -1.0f;
				LocalRotator.Roll *= -1.0f;
				FQuat4f NewRotator = LocalRotator.Quaternion();

				// 手动插值
				if (i == 0 && BoneRotationKeyFrame.Frame > 0)
				{
					while (CurrentFrame < BoneRotationKeyFrame.Frame)
					{
						RotationalKeys.Add(NewRotator);
						++CurrentFrame;
					}
				}
				else
				{
					while (CurrentFrame < BoneRotationKeyFrame.Frame)
					{
						uint32 LastFrame = KeyFrameBone.BoneRotations[i - 1].Frame;
						RotationalKeys.Add(
							FMath::Lerp(LastRotator, NewRotator,
							            static_cast<float>(CurrentFrame - LastFrame) /
							            static_cast<float>(BoneRotationKeyFrame.Frame - LastFrame)));
						++CurrentFrame;
					}
				}
				LastRotator = NewRotator;
				RotationalKeys.Add(NewRotator);
			}

			// 设置缩放关键帧
			CurrentFrame = 0;
			for (int32 i = 0; i < KeyFrameBone.BoneScale.Num(); ++i, ++CurrentFrame)
			{
				TWraithAnimFrame<FVector3f> BoneScaleKeyFrame = KeyFrameBone.BoneScale[i];

				// 手动插值
				if (i == 0 && BoneScaleKeyFrame.Frame > 0)
				{
					while (CurrentFrame < BoneScaleKeyFrame.Frame)
					{
						ScalingKeys.Add(BoneScaleKeyFrame.Value);
						++CurrentFrame;
					}
				}
				else
				{
					while (CurrentFrame < BoneScaleKeyFrame.Frame)
					{
						TWraithAnimFrame<FVector3f> LastScaleFrame = KeyFrameBone.BoneScale[i - 1];
						ScalingKeys.Add(
							FMath::Lerp(LastScaleFrame.Value, BoneScaleKeyFrame.Value,
							            static_cast<float>(CurrentFrame - LastScaleFrame.Frame) /
							            static_cast<float>(BoneScaleKeyFrame.Frame - LastScaleFrame.Frame)));
						++CurrentFrame;
					}
				}
				ScalingKeys.Add(BoneScaleKeyFrame.Value);
			}

			// 保证长度相同
			int32 ArrLen = FMath::Max3(PositionalKeys.Num(), RotationalKeys.Num(), ScalingKeys.Num());
			while (PositionalKeys.Num() < ArrLen)
			{
				if (PositionalKeys.IsEmpty())
				{
					PositionalKeys.Add(FVector3f::ZeroVector);
				}
				else
				{
					FVector3f LastItem = PositionalKeys.Last();
					PositionalKeys.Add(LastItem);
				}
			}
			while (RotationalKeys.Num() < ArrLen)
			{
				if (RotationalKeys.IsEmpty())
				{
					RotationalKeys.Add(FQuat4f());
				}
				else
				{
					FQuat4f LastItem = RotationalKeys.Last();
					RotationalKeys.Add(LastItem);
				}
			}
			while (ScalingKeys.Num() < ArrLen)
			{
				if (ScalingKeys.IsEmpty())
				{
					ScalingKeys.Add(FVector3f::OneVector);
				}
				else
				{
					FVector3f LastItem = ScalingKeys.Last();
					ScalingKeys.Add(LastItem);
				}
			}

			Controller.SetBoneTrackKeys(NewCurveName, PositionalKeys, RotationalKeys, ScalingKeys);
		}

		Controller.NotifyPopulated();
		Controller.CloseBracket();
		AnimSequence->Modify(true);
		AnimSequence->PostEditChange();
		FAssetRegistryModule::AssetCreated(AnimSequence);
		bool bDirty = AnimSequence->MarkPackageDirty();
	}
	if (AnimSequence)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		AssetEditorSubsystem->OpenEditorForAsset(AnimSequence);
	}

	return AnimSequence;
}

FMeshBoneInfo USeAnimAssetFactory::GetBone(const FString& AnimBoneName)
{
	for (const auto& Bone : Bones)
	{
		if (Bone.Name.ToString() == AnimBoneName)
		{
			return Bone;
		}
	}
	return FMeshBoneInfo();
}

int USeAnimAssetFactory::GetBoneIndex(const FString& AnimBoneName)
{
	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		if (Bones[Index].Name.ToString() == AnimBoneName)
		{
			return Index;
		}
	}
	return -69;
}

#undef LOCTEXT_NAMESPACE
