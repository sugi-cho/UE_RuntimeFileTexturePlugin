#include "RuntimeFileTextureInternal.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Engine/Texture2D.h"

namespace
{
	const TSet<FString>& ImageExtensions()
	{
		static const TSet<FString> Extensions = {
			TEXT("png"), TEXT("jpg"), TEXT("jpeg"), TEXT("bmp"), TEXT("tga"), TEXT("exr")
		};
		return Extensions;
	}

	const TSet<FString>& VideoExtensions()
	{
		static const TSet<FString> Extensions = {
			TEXT("mp4"), TEXT("mov"), TEXT("wmv"), TEXT("avi"), TEXT("m4v")
		};
		return Extensions;
	}
}

bool RuntimeFileTextureInternal::IsSupportedImageFile(const FString& FilePath)
{
	return ImageExtensions().Contains(FPaths::GetExtension(FilePath).ToLower());
}

bool RuntimeFileTextureInternal::IsSupportedVideoFile(const FString& FilePath)
{
	return VideoExtensions().Contains(FPaths::GetExtension(FilePath).ToLower());
}

bool RuntimeFileTextureInternal::IsSupportedFile(const FString& FilePath)
{
	return IsSupportedImageFile(FilePath) || IsSupportedVideoFile(FilePath);
}

UTexture2D* RuntimeFileTextureInternal::LoadImageTexture(UObject* Outer, const FString& FilePath, FString& OutError)
{
	OutError.Reset();

	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
	{
		OutError = TEXT("Failed to load image as Texture2D.");
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
	const EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(FileData.GetData(), FileData.Num());
	if (ImageFormat == EImageFormat::Invalid)
	{
		OutError = TEXT("Failed to load image as Texture2D.");
		return nullptr;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
	{
		OutError = TEXT("Failed to load image as Texture2D.");
		return nullptr;
	}

	const int32 Width = ImageWrapper->GetWidth();
	const int32 Height = ImageWrapper->GetHeight();
	if (Width <= 0 || Height <= 0)
	{
		OutError = TEXT("Failed to load image as Texture2D.");
		return nullptr;
	}

	const bool bUseFloat = ImageFormat == EImageFormat::EXR;
	const EPixelFormat PixelFormat = bUseFloat ? PF_FloatRGBA : PF_B8G8R8A8;

	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PixelFormat);
	if (!Texture)
	{
		OutError = TEXT("Failed to load image as Texture2D.");
		return nullptr;
	}

	Texture->NeverStream = true;
	Texture->SRGB = !bUseFloat;
	Texture->MipGenSettings = TMGS_NoMipmaps;
	Texture->CompressionSettings = bUseFloat ? TC_HDR : TC_Default;

	if (bUseFloat)
	{
		TArray64<uint8> RawData;
		if (!ImageWrapper->GetRaw(ERGBFormat::RGBAF, 16, RawData))
		{
			OutError = TEXT("Failed to load image as Texture2D.");
			return nullptr;
		}

		void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
		Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	}
	else
	{
		TArray64<uint8> RawData;
		if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
		{
			OutError = TEXT("Failed to load image as Texture2D.");
			return nullptr;
		}

		void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
		Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	}

	Texture->UpdateResource();
	return Texture;
}
