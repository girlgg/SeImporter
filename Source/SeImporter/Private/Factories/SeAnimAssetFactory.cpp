// Fill out your copyright notice in the Description page of Project Settings.


#include "Factories/SeAnimAssetFactory.h"

#include "Interfaces/IMainFrameModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Serialization/LargeMemoryReader.h"
#include "..\..\Public\Structures\SeAnim.h"
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

UObject* USeAnimAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
                                                const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn,
                                                bool& bOutOperationCanceled)
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
			const FBoneInfo& KeyFrameBone = Anim->BonesInfos[BoneIndex];
			auto BoneMesh = GetBone(KeyFrameBone.Name);
			if (BoneMesh.Name == "None") { continue; }
			// Add Bone Transform curve, and add base pose transform to start
			FSmartName NewCurveName;
			NewCurveName.DisplayName = FName(KeyFrameBone.Name);

			FAnimationCurveIdentifier TransformCurveId(NewCurveName, ERawCurveTrackTypes::RCT_Transform);
			AnimSequence->GetController().AddCurve(TransformCurveId, AACF_DriveTrack | AACF_Editable);

			// Loop thru actual bone positions
			if (KeyFrameBone.BonePositions.Num() > 0)
			{
				TArray<TArray<FRichCurveKey>> PosCurveKeys;
				TArray<FRichCurveKey> PositionKeysZ;
				TArray<FRichCurveKey> PositionKeysY;
				TArray<FRichCurveKey> PositionKeysX;

				for (size_t i = 0; i < KeyFrameBone.BonePositions.Num(); i++)
				{
					FVector boneFrameVector;
					auto BonePosAnimFrame = KeyFrameBone.BonePositions[i];
					BonePosAnimFrame.Value[1] *= -1;
					auto TimeInSeconds = static_cast<float>(BonePosAnimFrame.Frame) / Anim->Header.FrameRate;

					boneFrameVector.X = BonePosAnimFrame.Value[0];
					boneFrameVector.Y = BonePosAnimFrame.Value[1];
					boneFrameVector.Z = BonePosAnimFrame.Value[2];

					PositionKeysX.Add(FRichCurveKey(TimeInSeconds, boneFrameVector.X));
					PositionKeysY.Add(FRichCurveKey(TimeInSeconds, boneFrameVector.Y));
					PositionKeysZ.Add(FRichCurveKey(TimeInSeconds, boneFrameVector.Z));
				}
				PosCurveKeys.Add(PositionKeysX);
				PosCurveKeys.Add(PositionKeysY);
				PosCurveKeys.Add(PositionKeysZ);
				for (int i = 0; i < 3; ++i)
				{
					const EVectorCurveChannel Axis = static_cast<EVectorCurveChannel>(i);
					UAnimationCurveIdentifierExtensions::GetTransformChildCurveIdentifier(
						TransformCurveId, ETransformCurveChannel::Position, Axis);
					AnimSequence->GetController().SetCurveKeys(TransformCurveId, PosCurveKeys[i], bShouldTransact);
				}
			}
			if (KeyFrameBone.BoneRotations.Num() > 0)
			{
				TArray<TArray<FRichCurveKey>> RotCurveKeys;
				TArray<FRichCurveKey> RotationKeysZ;
				TArray<FRichCurveKey> RotationKeysY;
				TArray<FRichCurveKey> RotationKeysX;

				for (size_t i = 0; i < KeyFrameBone.BoneRotations.Num(); i++)
				{
					auto BoneRotationKeyFrame = KeyFrameBone.BoneRotations[i];
					// Unreal uses other axis type than COD engine
					FRotator3f LocalRotator = BoneRotationKeyFrame.Value.Rotator();
					LocalRotator.Yaw *= -1.0f;
					LocalRotator.Roll *= -1.0f;

					auto TimeInSeconds = static_cast<float>(BoneRotationKeyFrame.Frame) / Anim->Header.FrameRate;
					RotationKeysX.Add(FRichCurveKey(TimeInSeconds, LocalRotator.Pitch));
					RotationKeysY.Add(FRichCurveKey(TimeInSeconds, LocalRotator.Yaw));
					RotationKeysZ.Add(FRichCurveKey(TimeInSeconds, LocalRotator.Roll));
				}
				RotCurveKeys.Add(RotationKeysZ);
				RotCurveKeys.Add(RotationKeysX);
				RotCurveKeys.Add(RotationKeysY);
				for (int i = 0; i < 3; ++i)
				{
					const EVectorCurveChannel Axis = static_cast<EVectorCurveChannel>(i);
					UAnimationCurveIdentifierExtensions::GetTransformChildCurveIdentifier(
						TransformCurveId, ETransformCurveChannel::Rotation, Axis);
					AnimSequence->GetController().SetCurveKeys(TransformCurveId, RotCurveKeys[i], bShouldTransact);
				}
			}
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
