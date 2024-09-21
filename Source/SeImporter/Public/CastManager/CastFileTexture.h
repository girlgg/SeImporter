#pragma once

class FCastFileTexture
{
public:
	FORCEINLINE FString GetFileName() { return TextureFileName; }
	void SetName(const FString& InName) { TextureFileName = InName; }

private:
	FString TextureFileName;
};
