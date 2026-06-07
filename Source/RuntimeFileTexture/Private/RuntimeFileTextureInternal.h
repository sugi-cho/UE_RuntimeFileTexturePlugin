#pragma once

#include "CoreMinimal.h"

class UTexture2D;

namespace RuntimeFileTextureInternal
{
	bool SelectFileDialog(FString& OutFilePath);
	bool IsSupportedImageFile(const FString& FilePath);
	bool IsSupportedVideoFile(const FString& FilePath);
	bool IsSupportedFile(const FString& FilePath);
	UTexture2D* LoadImageTexture(UObject* Outer, const FString& FilePath, FString& OutError);
}
