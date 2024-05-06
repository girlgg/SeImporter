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

	Bones = Skeleton->GetReferenceSkeleton().GetRawRefBoneInfo();//获取骨骼代号
	BonePoses = Skeleton->GetReferenceSkeleton().GetRawRefBonePose();     //获取默认Pose      pose就是一个变换数组第几个就代表地几个骨骼。。。。。。
	
	Bones = Skeleton->GetReferenceSkeleton().GetRawRefBoneInfo();
	if (TArray64<uint8> FileDataOld; FFileHelper::LoadFileToArray(FileDataOld, *Filename))
	{
		FLargeMemoryReader Reader(FileDataOld.GetData(), FileDataOld.Num());
		FSeAnim* Anim = new FSeAnim();
		Anim->ParseAnim(Reader);

		if (SettingsImporter->bOverrideAnimType)
		{
			Anim->Header.AnimType = SettingsImporter->AnimType;/////////////////////////这里将设置里的数据提取出来
			///这里将设置里的数据提取出来
		}

		
		Controller.SetFrameRate(FFrameRate(Anim->Header.FrameRate, 1), bShouldTransact);
		Controller.SetNumberOfFrames(FFrameNumber(int(Anim->Header.FrameCountBuffer)), bShouldTransact);
		UE_LOG(LogTemp, Warning, TEXT("This animation '%s' is of type %s"), *Filename,
		       *UEnum::GetValueAsString(Anim->Header.AnimType));

		
		///函数更改部分。。。。。。。。。。。。。。。。。。。。。。。。。下面的全改了。。。。。。。。。。。。。。。。。。。。。。
		
		
        
        bool UseCurveSave=SettingsImporter->bUseCurveeSave;
	TObjectPtr<UAnimSequence> RefposeSequence=SettingsImporter->RefposeSequence;

        for (int32 BoneTreeIndex = 0; BoneTreeIndex < Bones.Num(); BoneTreeIndex++)
        {
            const FName BoneTreeName = Skeleton->GetReferenceSkeleton().GetBoneName(BoneTreeIndex);    //从数子获取

           FTransform RePoseTransform;                                                                              
           if(SettingsImporter->RefposeSequence&&SettingsImporter->RefposeSequence->GetSkeleton()==SettingsImporter->Skeleton)  //重载姿势用的
           {
               FAnimPoseEvaluationOptions EvaluationOptions;
               FAnimPose Pose;
               FName BoneName=BoneTreeName;
               UAnimPoseExtensions::GetAnimPoseAtFrame( SettingsImporter->RefposeSequence, SettingsImporter->PoseTime,EvaluationOptions,Pose);
               RePoseTransform =UAnimPoseExtensions::GetBonePose(Pose,BoneName,EAnimPoseSpaces::Local);
           }
            else
            {
                RePoseTransform=BonePoses[BoneTreeIndex];
            }


            Controller.AddBoneCurve(BoneTreeName, bShouldTransact);//这个一定要有！！
            if(UseCurveSave)//添加曲线插槽（原版）
            {
                FAnimationCurveIdentifier TransformCurveId(BoneTreeName, ERawCurveTrackTypes::RCT_Transform);           
                AnimSequence->GetController().AddCurve(TransformCurveId, AACF_DriveTrack | AACF_Editable);// 这里才加
                if (!(Anim->Header.AnimType == ESeAnimAnimationType::SEANIM_ABSOLUTE ))
                    AnimSequence->GetController().SetTransformCurveKey(TransformCurveId,0,RePoseTransform);
            }
          
            /*              ///////////////////////这里改成所有的骨骼都加一遍曲线来还原refpose                            */
            
            if(!(Anim->Header.AnimType == ESeAnimAnimationType::SEANIM_ABSOLUTE )&&!UseCurveSave)
            {

                TArray<FVector3f> PositionalKeys;
                TArray<FQuat4f> RotationalKeys;
                TArray<FVector3f> ScalingKeys;
                int32 longer=AnimSequence->GetNumberOfSampledKeys();
                for(int i=0;i<=longer;i++)
                {
                    PositionalKeys.Add(FVector3f(RePoseTransform.GetLocation()));
                    RotationalKeys.Add(FQuat4f(RePoseTransform.GetRotation()));
                    ScalingKeys.Add(FVector3f(1,1,1));

                }
                Controller.SetBoneTrackKeys(BoneTreeName, PositionalKeys, RotationalKeys, ScalingKeys);
               
            }
            /*                          /////////////////////////////////////////////                                   */
        }

        
        //开始添加曲线循环遍历Seanim数据的所有骨骼
        for (int32 BoneIndex = 0; BoneIndex < Anim->BonesInfos.Num(); BoneIndex++)
        {
            const FBoneInfo& KeyFrameBone = Anim->BonesInfos[BoneIndex];   //
            auto BoneMesh = GetBone(KeyFrameBone.Name);
            if (BoneMesh.Name == "None") { continue; }
            
            // Add Bone Transform curve, and add base pose transform to start

         
            FName NewCurveName(KeyFrameBone.Name);//////////////////////##################################
            TArray<FVector3f> PositionalKeys;////////////////////////######################################
            TArray<FQuat4f> RotationalKeys;///////////////////////////////####################################
            TArray<FVector3f> ScalingKeys;/////////////////////////#################################
            uint32 CurrentFrame = 0;///#######################

       ///设置RefPose给后面的数据用##################################
            
          FTransform BonePoseTransform;
            if(SettingsImporter->RefposeSequence)
            {
                FAnimPoseEvaluationOptions EvaluationOptions;
                FAnimPose Pose;
                FName BoneName=FName(KeyFrameBone.Name);
                UAnimPoseExtensions::GetAnimPoseAtFrame( SettingsImporter->RefposeSequence, SettingsImporter->PoseTime,EvaluationOptions,Pose);
                BonePoseTransform =UAnimPoseExtensions::GetBonePose(Pose,BoneName,EAnimPoseSpaces::Local);
            }
            else
            {
                BonePoseTransform = BonePoses[GetBoneIndex(KeyFrameBone.Name)];  //从refPose里找到该骨骼的默认变换.。。。。。。
            }
          
 

            
    
            /*                        UE4                                     */
            //FSmartName NewCurveName;
          // Skeleton->AddSmartNameAndModify(USkeleton::AnimTrackCurveMappingName, FName(KeyFrameBone.Name), NewCurveName); //这个本来呢获得骨骼名字的不知为啥弃用了。。。。。。
          //  Skeleton->AddCurveMetaData(FName(KeyFrameBone.Name));
          //  FAnimationCurveIdentifier TransformCurveId(NewCurveName, ERawCurveTrackTypes::RCT_Transform);

            /*                        UE5                                   *///############################################################
        

            FAnimationCurveIdentifier TransformCurveId(FName(KeyFrameBone.Name), ERawCurveTrackTypes::RCT_Transform);
            if(UseCurveSave)
            {
                Skeleton->AddCurveMetaData(FName(KeyFrameBone.Name));//骨骼ID//////////////////////////////////////////////////////////////
                AnimSequence->GetController().AddCurve(TransformCurveId, AACF_DriveTrack | AACF_Editable);
            }
               
            
            

            
           //已弃用无法还原完整的refpose!!!!!!!!功能以放在上方。。。
          // AnimSequence->GetController().SetTransformCurveKey(TransformCurveId,0,BonePoseTransform);  //将第0针设为默认Pose的位置///////这里默认Pose变为曲线姿势有问题。。。。。。//这里有问题不会变量所有的骨骼姿势
            // Loop thru actual bone positions   循环到实际骨骼位置
            if (KeyFrameBone.BonePositions.Num() > 0)
            {
                TArray<TArray<FRichCurveKey>> PosCurveKeys;
                TArray<FRichCurveKey> PositionKeysZ;
                TArray<FRichCurveKey> PositionKeysY;
                TArray<FRichCurveKey> PositionKeysX;
                
                FVector relative_transform= FVector(0,0,0);  //Anim是自己定义用来缓存Seanim的类
             
                if (Anim->Header.AnimType == ESeAnimAnimationType::SEANIM_ABSOLUTE )
                {
                    relative_transform = FVector(0,0,0);
                }
                else
                {
                    relative_transform = BonePoseTransform.GetLocation(); ///????
                }

                /////////////////////////////###############################################################3    曲线法
                if(UseCurveSave)
                {
                    for (size_t i = 0; i < KeyFrameBone.BonePositions.Num(); i++)   //动画关键帧数
                    {
                        FVector boneFrameVector;
                        auto BonePosAnimFrame = KeyFrameBone.BonePositions[i];    //获取提取动画的位移
                        BonePosAnimFrame.Value[1] *= -1;
                        auto TimeInSeconds = static_cast<float>(BonePosAnimFrame.Frame) / Anim->Header.FrameRate;
                    
                        // Relative_transform should be 0.0.0 if absolute anim.. its needed for relative/delta/additive  如果绝对动画，Relative_transform应为 0.0.0。相对/增量/添加剂需要它
                        boneFrameVector.X = BonePosAnimFrame.Value[0] + relative_transform[0];//
                        boneFrameVector.Y = BonePosAnimFrame.Value[1] + relative_transform[1];
                        boneFrameVector.Z = BonePosAnimFrame.Value[2] + relative_transform[2];

                  
                    
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
                        UAnimationCurveIdentifierExtensions::GetTransformChildCurveIdentifier(TransformCurveId, ETransformCurveChannel::Position, Axis);
                        AnimSequence->GetController().SetCurveKeys(TransformCurveId, PosCurveKeys[i], bShouldTransact);
                    }
                }
                else
                {
                    /////////////////////////////////////////////////////////////###################################        骨骼法
                    /// relative_transform = BonePoseTransform.GetLocation();
                    CurrentFrame = 0;
                    for (int32 i = 0; i < KeyFrameBone.BonePositions.Num(); ++i,  ++CurrentFrame)  //遍历该骨骼所有的关键针
                    {
                        FVector3f boneFrameVector,LastBoneFrameVector;
                        auto BonePosAnimFrame = KeyFrameBone.BonePositions[i];    //获取提取动画的位移
                        BonePosAnimFrame.Value[1] *= -1;
                    
                       
                        boneFrameVector.X = BonePosAnimFrame.Value[0] + relative_transform[0];//
                        boneFrameVector.Y = BonePosAnimFrame.Value[1] + relative_transform[1];
                        boneFrameVector.Z = BonePosAnimFrame.Value[2] + relative_transform[2];
                    
                        // 手动插值
                        if (i == 0 && BonePosAnimFrame.Frame > 0)
                        {
                           //开始设置下一针
                            while (CurrentFrame < BonePosAnimFrame.Frame)
                            {
                                PositionalKeys.Add(boneFrameVector);
                                ++CurrentFrame;
                            }
                        }
                        else
                        {
                            while (CurrentFrame < BonePosAnimFrame.Frame)//循环到下一关键针的时间   Frame是该位置在轨道上的那一针上   
                            {
                                TWraithAnimFrame<FVector3f> LastBonePosAnimFrame = KeyFrameBone.BonePositions[i - 1];
                                PositionalKeys.Add(
                                    FMath::Lerp(LastBoneFrameVector,  boneFrameVector,
                                                static_cast<float>(CurrentFrame - LastBonePosAnimFrame.Frame) /
                                                static_cast<float>(BonePosAnimFrame.Frame - LastBonePosAnimFrame.Frame)));
                                ++CurrentFrame;
                            }
                        }
                        LastBoneFrameVector=boneFrameVector; 
                        PositionalKeys.Add(FVector3f(boneFrameVector));
                    }
                }
            }
                
            /////////////////////////////////////////////////////////////////////////##############################################

            //旋转
            if (KeyFrameBone.BoneRotations.Num() > 0)
            {
                TArray<TArray<FRichCurveKey>> RotCurveKeys;
                TArray<FRichCurveKey> RotationKeysZ;
                TArray<FRichCurveKey> RotationKeysY;
                TArray<FRichCurveKey> RotationKeysX;

                
                FQuat Rel_Rotation;
                if (Anim->Header.AnimType == ESeAnimAnimationType::SEANIM_ADDITIVE)
                {
                    Rel_Rotation =  BonePoseTransform.GetRotation();
                }
                else
                {
                    Rel_Rotation = FQuat(0, 0, 0, 1);
                }
               ///////////////////////////////////////////////////////////########################################曲线法
               if(UseCurveSave)
               {
                   for (size_t i = 0; i < KeyFrameBone.BoneRotations.Num(); i++)
                   {
                       auto BoneRotationKeyFrame = KeyFrameBone.BoneRotations[i];
                       BoneRotationKeyFrame.Value *=  FQuat4f(Rel_Rotation);//////////？？？------------------
                       // Unreal uses other axis type than COD engine  虚幻引擎使用COD引擎以外的其他轴类型
                       FRotator3f LocalRotator = BoneRotationKeyFrame.Value.Rotator(); ///只有addtive有用
                       LocalRotator.Yaw *= -1.0f;
                       LocalRotator.Roll *= -1.0f;
              
                       BoneRotationKeyFrame.Value = LocalRotator.Quaternion();
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
                       UAnimationCurveIdentifierExtensions::GetTransformChildCurveIdentifier(TransformCurveId, ETransformCurveChannel::Rotation, Axis);
                       AnimSequence->GetController().SetCurveKeys(TransformCurveId, RotCurveKeys[i], bShouldTransact);
                   }
               }
                ///////////////////////////////////###########################################################骨骼法
                ///
                ///

               else
               {
                   FQuat4f LastRotator;
                   CurrentFrame = 0;
                   for (int32 i = 0; i < KeyFrameBone.BoneRotations.Num(); ++i, ++CurrentFrame)
                   {
                      TWraithAnimFrame<FQuat4f> BoneRotationKeyFrame = KeyFrameBone.BoneRotations[i];
                       BoneRotationKeyFrame.Value *=  FQuat4f(Rel_Rotation);//添加旋转
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
               }
                //////////////////////////////////##################################################################
                ///
                ///

////////////////////////##############################################缩放
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
////////////////////////////################################


             
                
                int32 ArrLen = FMath::Max3(PositionalKeys.Num(), RotationalKeys.Num(), ScalingKeys.Num());
                while (PositionalKeys.Num() < ArrLen)
                {
                    if (PositionalKeys.IsEmpty())
                    {
                        bool IsAb=Anim->Header.AnimType == ESeAnimAnimationType::SEANIM_ABSOLUTE ;
                        PositionalKeys.Add(IsAb  ?  FVector3f::ZeroVector   :   FVector3f(BonePoseTransform.GetLocation()));//问题出在这里。。。FVector3f::ZeroVector
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
                        RotationalKeys.Add(FQuat4f(BonePoseTransform.GetRotation()));
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
