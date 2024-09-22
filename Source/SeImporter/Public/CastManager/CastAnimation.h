#pragma once

struct FCastCurveInfo
{
	FString NodeName;
	FString KeyPropertyName;
	TArray<uint32> KeyFrameBuffer;
	TArray<FVariant> KeyValueBuffer;
	FString Mode;
	float AdditiveBlendWeight;
};

struct FCastCurveModeOverrideInfo
{
	FString NodeName;
	FString Mode;
	bool OverrideTranslationCurves;
	bool OverrideRotationCurves;
	bool OverrideScaleCurves;
};

struct FCastNotificationTrackInfo
{
	FString Name;
	TArray<uint32> KeyFrameBuffer;
};

struct FCastAnimationInfo
{
	FString Name{""};
	float Framerate{30};
	bool Looping{false};

	TArray<FCastCurveInfo> Curves;
	TArray<FCastCurveModeOverrideInfo> CurveModeOverrides;
	TArray<FCastNotificationTrackInfo> NotificationTracks;
};
