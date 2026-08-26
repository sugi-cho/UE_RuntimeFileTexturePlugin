#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class UMediaPlayer;

namespace RuntimeFileTextureInternal
{
	const FName& GetDesiredMediaPlayerName();
	void ConfigureMediaPlayer(UMediaPlayer* MediaPlayer);
	bool SelectFileDialog(FString& OutFilePath);
	bool IsSupportedImageFile(const FString& FilePath);
	bool IsSupportedVideoFile(const FString& FilePath);
	bool IsSupportedFile(const FString& FilePath);
	UTexture2D* LoadImageTexture(UObject* Outer, const FString& FilePath, FString& OutError);
	UTexture2D* LoadTiffTexture(UObject* Outer, const FString& FilePath, FString& OutError);
}
