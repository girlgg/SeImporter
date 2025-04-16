#pragma once

enum class ECastAnimationType : uint8
{
	// Animation translations are set to this exact value each frame
	Absolute = 0,
	// This animation is applied to existing animation data in the scene (and is absolute)
	Additive = 1,
	// Animation translations are based on rest position in scene
	Relative = 2,
	// This animation is relative and contains delta data (Whole model movement) Delta tag name must be set!
	Delta = 3,
};

struct FCastCurveModeOverrideInfo
{
	FString NodeName;
	FString Mode;
	bool OverrideTranslationCurves;
	bool OverrideRotationCurves;
	bool OverrideScaleCurves;
};

struct FCastCurveInfo
{
	FString NodeName;
	// 可选类型 "rq", "tx", "ty", "tz", "sx", "sy", "sz", "bs", "vb"
	FString KeyPropertyName;
	TArray<uint32> KeyFrameBuffer;
	TArray<FVariant> KeyValueBuffer;
	FString Mode;
	float AdditiveBlendWeight = 1.f;

	/**
	 * @brief 将帧缓冲和值缓冲转换为 TMap 格式。
	 * @return TMap<uint32, FVariant> 帧号到键值的映射。如果缓冲大小不匹配则返回空 Map。
	 */
	TMap<uint32, FVariant> GetKeysAsMap() const
	{
		TMap<uint32, FVariant> KeyMap;
		if (KeyFrameBuffer.Num() == KeyValueBuffer.Num())
		{
			for (int32 i = 0; i < KeyFrameBuffer.Num(); ++i)
			{
				KeyMap.Add(KeyFrameBuffer[i], KeyValueBuffer[i]);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning,
			       TEXT(
				       "FCastCurveInfo::GetKeysAsMap: KeyFrameBuffer (%d) and KeyValueBuffer (%d) size mismatch for Node '%s' Property '%s'."
			       ), KeyFrameBuffer.Num(), KeyValueBuffer.Num(), *NodeName, *KeyPropertyName);
		}
		return KeyMap;
	}

	/**
	 * @brief 使用 TMap<uint32, FVariant> 设置帧缓冲和值缓冲。
	 * 会清空现有的缓冲。Map 会按键（帧号）排序后填充缓冲。
	 * @param KeyMap 帧号到键值的映射。
	 */
	void SetKeysFromMap(const TMap<uint32, FVariant>& KeyMap)
	{
		KeyFrameBuffer.Empty(KeyMap.Num());
		KeyValueBuffer.Empty(KeyMap.Num());

		for (const auto& Pair : KeyMap)
		{
			KeyFrameBuffer.Add(Pair.Key);
			KeyValueBuffer.Add(Pair.Value);
		}
	}

	/**
	 * @brief 添加一个单独的关键帧。
	 * 注意：不保证帧顺序，建议使用 SetKeysFromMap 或在添加完所有帧后手动排序。
	 * @param Frame 帧号
	 * @param Value 键值 (FVariant)
	 */
	void AddKey(uint32 Frame, const FVariant& Value)
	{
		// 这里可以添加检查，确保 Value 类型与 KeyPropertyName 匹配，但这会增加复杂性
		// 例如: if (KeyPropertyName == TEXT("rq") && !Value.IsType<FVector4>()) { /* 错误处理 */ }
		KeyFrameBuffer.Add(Frame);
		KeyValueBuffer.Add(Value);
	}
};

struct FCastNotificationTrackInfo
{
	FString Name;
	TArray<uint32> KeyFrameBuffer;

	/**
	 * @brief 获取所有通知帧的集合。
	 * @return TSet<uint32> 包含所有通知帧号的集合。
	 */
	TSet<uint32> GetNotificationFrames() const
	{
		TSet<uint32> Frames;
		for (uint32 Frame : KeyFrameBuffer)
		{
			Frames.Add(Frame);
		}
		return Frames;
	}

	/**
	 * @brief 使用 TSet<uint32> 设置通知帧。
	 * 会清空现有的缓冲。Set 会被排序后填充缓冲。
	 * @param Frames 包含所有通知帧号的集合。
	 */
	void SetNotificationFrames(const TSet<uint32>& Frames)
	{
		KeyFrameBuffer.Empty(Frames.Num());
		TArray<uint32> SortedFrames = Frames.Array();
		SortedFrames.Sort();
		KeyFrameBuffer.Append(SortedFrames);
	}

	/**
	 * @brief 添加一个通知帧。
	 * 注意：不保证帧顺序，建议使用 SetNotificationFrames 或在添加完所有帧后手动排序。
	 * @param Frame 帧号
	 */
	void AddNotificationKey(uint32 Frame)
	{
		if (!KeyFrameBuffer.Contains(Frame))
		{
			KeyFrameBuffer.Add(Frame);
		}
	}
};

struct FCastAnimationInfo
{
	FString Name{""};
	float Framerate{30.f};
	bool Looping{false};
	ECastAnimationType Type{ECastAnimationType::Absolute};
	FString DeltaTagName{""};
	TArray<FCastCurveInfo> Curves;
	TArray<FCastCurveModeOverrideInfo> CurveModeOverrides;
	TArray<FCastNotificationTrackInfo> NotificationTracks;

private:
	/**
	 * @brief 查找或创建指定节点和属性的曲线。
	 * @param NodeName 节点名。
	 * @param KeyPropertyName 属性名 ("rq", "tx", etc.)。
	 * @param DefaultMode 新创建曲线时的默认模式。
	 * @return 指向找到或创建的 FCastCurveInfo 的指针。如果输入无效则返回 nullptr。
	 */
	FCastCurveInfo* FindOrAddCurve(const FString& NodeName, const FString& KeyPropertyName,
	                               const FString& DefaultMode = TEXT("linear"))
	{
		if (NodeName.IsEmpty() || KeyPropertyName.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("FindOrAddCurve: Invalid NodeName or KeyPropertyName provided."));
			return nullptr;
		}

		// 尝试查找现有曲线
		for (FCastCurveInfo& Curve : Curves)
		{
			if (Curve.NodeName == NodeName && Curve.KeyPropertyName == KeyPropertyName)
			{
				return &Curve;
			}
		}

		// 如果未找到，创建新曲线并添加到数组
		FCastCurveInfo NewCurve;
		NewCurve.NodeName = NodeName;
		NewCurve.KeyPropertyName = KeyPropertyName;
		NewCurve.Mode = DefaultMode;
		Curves.Add(NewCurve);
		return &Curves.Last();
	}

	/**
	 * @brief 查找或创建指定名称的通知轨道。
	 * @param TrackName 通知轨道名称。
	 * @return 指向找到或创建的 FCastNotificationTrackInfo 的指针。如果输入无效则返回 nullptr。
	 */
	FCastNotificationTrackInfo* FindOrAddNotificationTrack(const FString& TrackName)
	{
		if (TrackName.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("FindOrAddNotificationTrack: Invalid TrackName provided."));
			return nullptr;
		}

		// 尝试查找现有轨道
		for (FCastNotificationTrackInfo& Track : NotificationTracks)
		{
			if (Track.Name == TrackName)
			{
				return &Track;
			}
		}

		// 如果未找到，创建新轨道并添加到数组
		FCastNotificationTrackInfo NewTrack;
		NewTrack.Name = TrackName;
		NotificationTracks.Add(NewTrack);
		return &NotificationTracks.Last();
	}

public:
	/**
	 * @brief 为指定节点添加旋转关键帧。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param Rotation FVector4 (XYZW，确保一致)。
	 */
	void AddRotationKey(const FString& NodeName, uint32 Frame, const FVector4& Rotation)
	{
		if (FCastCurveInfo* Curve = FindOrAddCurve(NodeName, TEXT("rq")))
		{
			Curve->AddKey(Frame, FVariant(Rotation));
		}
	}

	/**
	 * @brief 为指定节点添加位移关键帧 (一次性添加 X, Y, Z)。
	 * 这会查找/创建 tx, ty, tz 三条曲线。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param Translation FVector (X, Y, Z 位移)。
	 */
	void AddTranslationKey(const FString& NodeName, uint32 Frame, const FVector& Translation)
	{
		if (FCastCurveInfo* CurveX = FindOrAddCurve(NodeName, TEXT("tx")))
		{
			CurveX->AddKey(Frame, FVariant(static_cast<float>(Translation.X)));
		}
		if (FCastCurveInfo* CurveY = FindOrAddCurve(NodeName, TEXT("ty")))
		{
			CurveY->AddKey(Frame, FVariant(static_cast<float>(Translation.Y)));
		}
		if (FCastCurveInfo* CurveZ = FindOrAddCurve(NodeName, TEXT("tz")))
		{
			CurveZ->AddKey(Frame, FVariant(static_cast<float>(Translation.Z)));
		}
	}

	/**
	 * @brief 为指定节点添加 X 位移关键帧。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param Value X 位移值。
	 */
	void AddTranslationXKey(const FString& NodeName, uint32 Frame, float Value)
	{
		if (FCastCurveInfo* Curve = FindOrAddCurve(NodeName, TEXT("tx")))
		{
			Curve->AddKey(Frame, FVariant(Value));
		}
	}

	/**
	 * @brief 为指定节点添加 Y 位移关键帧。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param Value Y 位移值。
	 */
	void AddTranslationYKey(const FString& NodeName, uint32 Frame, float Value)
	{
		if (FCastCurveInfo* Curve = FindOrAddCurve(NodeName, TEXT("ty")))
		{
			Curve->AddKey(Frame, FVariant(Value));
		}
	}

	/**
	 * @brief 为指定节点添加 Z 位移关键帧。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param Value Z 位移值。
	 */
	void AddTranslationZKey(const FString& NodeName, uint32 Frame, float Value)
	{
		if (FCastCurveInfo* Curve = FindOrAddCurve(NodeName, TEXT("tz")))
		{
			Curve->AddKey(Frame, FVariant(Value));
		}
	}

	/**
	 * @brief 为指定节点添加缩放关键帧 (一次性添加 X, Y, Z)。
	 * 这会查找/创建 sx, sy, sz 三条曲线。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param Scale FVector (X, Y, Z 缩放)。
	 */
	void AddScaleKey(const FString& NodeName, uint32 Frame, const FVector& Scale)
	{
		if (FCastCurveInfo* CurveX = FindOrAddCurve(NodeName, TEXT("sx")))
		{
			CurveX->AddKey(Frame, FVariant(static_cast<float>(Scale.X)));
		}
		if (FCastCurveInfo* CurveY = FindOrAddCurve(NodeName, TEXT("sy")))
		{
			CurveY->AddKey(Frame, FVariant(static_cast<float>(Scale.Y)));
		}
		if (FCastCurveInfo* CurveZ = FindOrAddCurve(NodeName, TEXT("sz")))
		{
			CurveZ->AddKey(Frame, FVariant(static_cast<float>(Scale.Z)));
		}
	}

	/**
	 * @brief 为指定节点添加 X 缩放关键帧。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param Value X 缩放值。
	 */
	void AddScaleXKey(const FString& NodeName, uint32 Frame, float Value)
	{
		if (FCastCurveInfo* Curve = FindOrAddCurve(NodeName, TEXT("sx")))
		{
			Curve->AddKey(Frame, FVariant(Value));
		}
	}

	/**
	 * @brief 为指定节点添加 Y 缩放关键帧。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param Value Y 缩放值。
	 */
	void AddScaleYKey(const FString& NodeName, uint32 Frame, float Value)
	{
		if (FCastCurveInfo* Curve = FindOrAddCurve(NodeName, TEXT("sy")))
		{
			Curve->AddKey(Frame, FVariant(Value));
		}
	}

	/**
	 * @brief 为指定节点添加 Z 缩放关键帧。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param Value Z 缩放值。
	 */
	void AddScaleZKey(const FString& NodeName, uint32 Frame, float Value)
	{
		if (FCastCurveInfo* Curve = FindOrAddCurve(NodeName, TEXT("sz")))
		{
			Curve->AddKey(Frame, FVariant(Value));
		}
	}

	/**
	 * @brief 为指定名称添加 BlendShape 关键帧。
	 * NodeName 通常用于标识是哪个 BlendShape。
	 * @param BlendShapeNodeName BlendShape 的名称或标识符。
	 * @param Frame 帧号。
	 * @param Value BlendShape 权重 (通常是 0 到 1)。
	 */
	void AddBlendShapeKey(const FString& BlendShapeNodeName, uint32 Frame, float Value)
	{
		// KeyPropertyName 固定为 "bs"
		if (FCastCurveInfo* Curve = FindOrAddCurve(BlendShapeNodeName, TEXT("bs")))
		{
			Curve->AddKey(Frame, FVariant(Value));
		}
	}

	/**
	 * @brief 为指定节点添加可见性关键帧。
	 * @param NodeName 节点名称。
	 * @param Frame 帧号。
	 * @param bIsVisible true 表示可见 (对应 "i")，false 表示隐藏 (对应 "h")。
	 *                  注意：KeyValueBuffer 存储的是 FString("i") 或 FString("h")。
	 */
	void AddVisibilityKey(const FString& NodeName, uint32 Frame, bool bIsVisible)
	{
		if (FCastCurveInfo* Curve = FindOrAddCurve(NodeName, TEXT("vb")))
		{
			Curve->AddKey(Frame, FVariant(bIsVisible ? 1 : 0));
		}
	}

	/**
	 * @brief 为指定的通知轨道添加一个通知事件帧。
	 * @param TrackName 通知轨道的名称。
	 * @param Frame 帧号。
	 */
	void AddNotificationKey(const FString& TrackName, uint32 Frame)
	{
		if (FCastNotificationTrackInfo* Track = FindOrAddNotificationTrack(TrackName))
		{
			Track->AddNotificationKey(Frame);
		}
	}

	/**
	 * @brief 清理所有曲线和通知轨道。
	 */
	void ClearAllTracks()
	{
		Curves.Empty();
		NotificationTracks.Empty();
		CurveModeOverrides.Empty();
	}

	/**
	 * @brief 对所有曲线和通知轨道的关键帧进行排序。在添加完所有关键帧后调用此方法，确保帧按时间顺序排列。
	 */
	void SortAllKeyframes()
	{
		for (FCastCurveInfo& Curve : Curves)
		{
			if (Curve.KeyFrameBuffer.Num() != Curve.KeyValueBuffer.Num())
			{
				UE_LOG(LogTemp, Warning,
				       TEXT("SortAllKeyframes: Skipping curve '%s' for node '%s' due to buffer size mismatch."),
				       *Curve.KeyPropertyName, *Curve.NodeName);
				continue;
			}

			// 创建 Pair 数组用于排序
			TArray<TPair<uint32, FVariant>> KeyPairs;
			KeyPairs.Reserve(Curve.KeyFrameBuffer.Num());
			for (int32 i = 0; i < Curve.KeyFrameBuffer.Num(); ++i)
			{
				KeyPairs.Emplace(Curve.KeyFrameBuffer[i], Curve.KeyValueBuffer[i]);
			}

			// 按帧号排序
			KeyPairs.Sort([](const TPair<uint32, FVariant>& A, const TPair<uint32, FVariant>& B)
			{
				return A.Key < B.Key;
			});

			// 将排序后的结果写回 Buffer
			Curve.KeyFrameBuffer.Reset(KeyPairs.Num());
			Curve.KeyValueBuffer.Reset(KeyPairs.Num());
			for (const auto& Pair : KeyPairs)
			{
				Curve.KeyFrameBuffer.Add(Pair.Key);
				Curve.KeyValueBuffer.Add(Pair.Value);
			}
		}

		for (FCastNotificationTrackInfo& Track : NotificationTracks)
		{
			Track.KeyFrameBuffer.Sort();
			// 移除重复帧
			Track.KeyFrameBuffer = TSet(Track.KeyFrameBuffer).Array();
		}
	}
};
