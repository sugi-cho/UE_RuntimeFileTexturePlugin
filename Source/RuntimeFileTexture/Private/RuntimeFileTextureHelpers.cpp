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
			TEXT("png"), TEXT("jpg"), TEXT("jpeg"), TEXT("bmp"), TEXT("tga"), TEXT("exr"), TEXT("tif"), TEXT("tiff")
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

namespace
{
	bool IsTiffPath(const FString& FilePath)
	{
		const FString Extension = FPaths::GetExtension(FilePath).ToLower();
		return Extension == TEXT("tif") || Extension == TEXT("tiff");
	}
}

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"
#include <wincodec.h>
#include "Windows/HideWindowsPlatformTypes.h"

namespace
{
	bool LoadTiffWithWic(const FString& FilePath, UTexture2D*& OutTexture, FString& OutError)
	{
		OutTexture = nullptr;
		OutError.Reset();

		HRESULT CoInitResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool bShouldUninitialize = SUCCEEDED(CoInitResult);
		if (FAILED(CoInitResult) && CoInitResult != RPC_E_CHANGED_MODE)
		{
			OutError = TEXT("Failed to load TIFF image as Texture2D.");
			return false;
		}

		IWICImagingFactory* Factory = nullptr;
		const HRESULT FactoryHr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&Factory)
		);
		if (FAILED(FactoryHr) || Factory == nullptr)
		{
			if (bShouldUninitialize)
			{
				CoUninitialize();
			}
			OutError = TEXT("Failed to load TIFF image as Texture2D.");
			return false;
		}

		IWICBitmapDecoder* Decoder = nullptr;
		const HRESULT DecoderHr = Factory->CreateDecoderFromFilename(
			*FilePath,
			nullptr,
			GENERIC_READ,
			WICDecodeMetadataCacheOnDemand,
			&Decoder
		);
		if (FAILED(DecoderHr) || Decoder == nullptr)
		{
			Factory->Release();
			if (bShouldUninitialize)
			{
				CoUninitialize();
			}
			OutError = TEXT("Failed to load TIFF image as Texture2D.");
			return false;
		}

		IWICBitmapFrameDecode* Frame = nullptr;
		const HRESULT FrameHr = Decoder->GetFrame(0, &Frame);
		if (FAILED(FrameHr) || Frame == nullptr)
		{
			Decoder->Release();
			Factory->Release();
			if (bShouldUninitialize)
			{
				CoUninitialize();
			}
			OutError = TEXT("Failed to load TIFF image as Texture2D.");
			return false;
		}

		UINT Width = 0;
		UINT Height = 0;
		if (FAILED(Frame->GetSize(&Width, &Height)) || Width == 0 || Height == 0)
		{
			Frame->Release();
			Decoder->Release();
			Factory->Release();
			if (bShouldUninitialize)
			{
				CoUninitialize();
			}
			OutError = TEXT("Failed to load TIFF image as Texture2D.");
			return false;
		}

		IWICFormatConverter* Converter = nullptr;
		const HRESULT ConverterHr = Factory->CreateFormatConverter(&Converter);
		if (FAILED(ConverterHr) || Converter == nullptr)
		{
			Frame->Release();
			Decoder->Release();
			Factory->Release();
			if (bShouldUninitialize)
			{
				CoUninitialize();
			}
			OutError = TEXT("Failed to load TIFF image as Texture2D.");
			return false;
		}

		const HRESULT InitHr = Converter->Initialize(
			Frame,
			GUID_WICPixelFormat32bppBGRA,
			WICBitmapDitherTypeNone,
			nullptr,
			0.0,
			WICBitmapPaletteTypeCustom
		);
		if (FAILED(InitHr))
		{
			Converter->Release();
			Frame->Release();
			Decoder->Release();
			Factory->Release();
			if (bShouldUninitialize)
			{
				CoUninitialize();
			}
			OutError = TEXT("Failed to load TIFF image as Texture2D.");
			return false;
		}

		UTexture2D* Texture = UTexture2D::CreateTransient(static_cast<int32>(Width), static_cast<int32>(Height), PF_B8G8R8A8);
		if (!Texture)
		{
			Converter->Release();
			Frame->Release();
			Decoder->Release();
			Factory->Release();
			if (bShouldUninitialize)
			{
				CoUninitialize();
			}
			OutError = TEXT("Failed to load TIFF image as Texture2D.");
			return false;
		}

		Texture->NeverStream = true;
		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
#if WITH_EDITORONLY_DATA
		Texture->MipGenSettings = TMGS_NoMipmaps;
#endif

		TArray<uint8> RawData;
		RawData.SetNumUninitialized(static_cast<int32>(Width) * static_cast<int32>(Height) * 4);

		const UINT Stride = static_cast<UINT>(Width) * 4;
		const HRESULT CopyHr = Converter->CopyPixels(nullptr, Stride, static_cast<UINT>(RawData.Num()), RawData.GetData());
		if (FAILED(CopyHr))
		{
			Converter->Release();
			Frame->Release();
			Decoder->Release();
			Factory->Release();
			if (bShouldUninitialize)
			{
				CoUninitialize();
			}
			OutError = TEXT("Failed to load TIFF image as Texture2D.");
			return false;
		}

		void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
		Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
		Texture->UpdateResource();

		OutTexture = Texture;

		Converter->Release();
		Frame->Release();
		Decoder->Release();
		Factory->Release();
		if (bShouldUninitialize)
		{
			CoUninitialize();
		}
		return true;
	}
}

UTexture2D* RuntimeFileTextureInternal::LoadTiffTexture(UObject* Outer, const FString& FilePath, FString& OutError)
{
	(void)Outer;
	OutError.Reset();
	UTexture2D* Texture = nullptr;
	if (!LoadTiffWithWic(FilePath, Texture, OutError))
	{
		return nullptr;
	}
	return Texture;
}

#else

UTexture2D* RuntimeFileTextureInternal::LoadTiffTexture(UObject* Outer, const FString& FilePath, FString& OutError)
{
	(void)Outer;
	(void)FilePath;
	OutError = TEXT("TIFF loading is only supported on Windows.");
	return nullptr;
}

#endif

UTexture2D* RuntimeFileTextureInternal::LoadImageTexture(UObject* Outer, const FString& FilePath, FString& OutError)
{
	OutError.Reset();

	if (IsTiffPath(FilePath))
	{
		if (UTexture2D* TiffTexture = LoadTiffTexture(Outer, FilePath, OutError))
		{
			return TiffTexture;
		}

		if (!OutError.IsEmpty())
		{
			return nullptr;
		}
	}

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
	Texture->CompressionSettings = bUseFloat ? TC_HDR : TC_Default;
#if WITH_EDITORONLY_DATA
	Texture->MipGenSettings = TMGS_NoMipmaps;
#endif

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
