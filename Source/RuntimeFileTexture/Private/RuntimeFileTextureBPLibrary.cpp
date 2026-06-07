#include "RuntimeFileTextureBPLibrary.h"

#include "Components/MeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Misc/Paths.h"
#include "RuntimeFileTextureInternal.h"

bool URuntimeFileTextureBPLibrary::SelectFile(FString& OutFilePath)
{
	return RuntimeFileTextureInternal::SelectFileDialog(OutFilePath);
}

FRuntimeFileTextureResult URuntimeFileTextureBPLibrary::ApplyFileToMesh(
	UMeshComponent* TargetMesh,
	FName TextureParameterName,
	const FString& FilePath,
	int32 MaterialIndex,
	bool bLoopVideo)
{
	if (!TargetMesh)
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("TargetMesh is not set.");
		return Result;
	}

	if (TextureParameterName.IsNone())
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("TextureParameterName is not set.");
		return Result;
	}

	if (!RuntimeFileTextureInternal::IsSupportedFile(FilePath))
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = FString::Printf(TEXT("Unsupported file extension: %s"), *FPaths::GetExtension(FilePath));
		return Result;
	}

	UMaterialInterface* BaseMaterial = TargetMesh->GetMaterial(MaterialIndex);
	if (!BaseMaterial)
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("Failed to create dynamic material instance.");
		return Result;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, TargetMesh);
	if (!MID)
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("Failed to create dynamic material instance.");
		return Result;
	}
	TargetMesh->SetMaterial(MaterialIndex, MID);

	if (RuntimeFileTextureInternal::IsSupportedImageFile(FilePath))
	{
		FString Error;
		UTexture2D* Texture = RuntimeFileTextureInternal::LoadImageTexture(TargetMesh, FilePath, Error);
		if (!Texture)
		{
			FRuntimeFileTextureResult Result;
			Result.FilePath = FilePath;
			Result.Error = Error;
			return Result;
		}

		MID->SetTextureParameterValue(TextureParameterName, Texture);
		FRuntimeFileTextureResult Result;
		Result.bSuccess = true;
		Result.FilePath = FilePath;
		Result.Type = ERuntimeFileTextureType::Image;
		Result.Texture = Texture;
		return Result;
	}

	UMediaPlayer* MediaPlayer = NewObject<UMediaPlayer>(TargetMesh);
	UMediaTexture* MediaTexture = NewObject<UMediaTexture>(TargetMesh);
	UFileMediaSource* MediaSource = NewObject<UFileMediaSource>(TargetMesh);
	if (!MediaPlayer || !MediaTexture || !MediaSource)
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("Failed to open video media source.");
		return Result;
	}

	MediaSource->SetFilePath(FilePath);
	MediaPlayer->SetLooping(bLoopVideo);
	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->UpdateResource();
	MID->SetTextureParameterValue(TextureParameterName, MediaTexture);

	if (!MediaPlayer->OpenSource(MediaSource))
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("Failed to open video media source.");
		return Result;
	}

	MediaPlayer->Play();
	FRuntimeFileTextureResult Result;
	Result.bSuccess = true;
	Result.FilePath = FilePath;
	Result.Type = ERuntimeFileTextureType::Video;
	Result.Texture = MediaTexture;
	return Result;
}
