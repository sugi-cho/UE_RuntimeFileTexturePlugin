#include "RuntimeFileTextureBPLibrary.h"

#include "Components/MeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Misc/Paths.h"
#include "RuntimeFileTexturePlaybackSubsystem.h"
#include "RuntimeFileTextureInternal.h"

namespace
{
	UMaterialInstanceDynamic* GetOrCreateMID(UMeshComponent* TargetMesh, int32 MaterialIndex, FString& OutError)
	{
		OutError.Reset();

		if (!TargetMesh)
		{
			OutError = TEXT("TargetMesh is not set.");
			return nullptr;
		}

		if (UMaterialInstanceDynamic* ExistingMID = Cast<UMaterialInstanceDynamic>(TargetMesh->GetMaterial(MaterialIndex)))
		{
			return ExistingMID;
		}

		UMaterialInterface* BaseMaterial = TargetMesh->GetMaterial(MaterialIndex);
		if (!BaseMaterial)
		{
			OutError = TEXT("Failed to create dynamic material instance.");
			return nullptr;
		}

		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, TargetMesh);
		if (!MID)
		{
			OutError = TEXT("Failed to create dynamic material instance.");
			return nullptr;
		}

		TargetMesh->SetMaterial(MaterialIndex, MID);
		return MID;
	}

	FRuntimeFileTextureResult MakeErrorResult(const FString& FilePath, const FString& Error)
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = Error;
		return Result;
	}
}

bool URuntimeFileTextureBPLibrary::SelectFile(FString& OutFilePath)
{
	return RuntimeFileTextureInternal::SelectFileDialog(OutFilePath);
}

FRuntimeFileTextureResult URuntimeFileTextureBPLibrary::ApplyTextureToMesh(
	UMeshComponent* TargetMesh,
	UTexture* Texture,
	FName TextureParameterName,
	int32 MaterialIndex)
{
	if (!Texture)
	{
		return MakeErrorResult(TEXT(""), TEXT("Texture is not set."));
	}

	if (TextureParameterName.IsNone())
	{
		return MakeErrorResult(TEXT(""), TEXT("TextureParameterName is not set."));
	}

	FString Error;
	UMaterialInstanceDynamic* MID = GetOrCreateMID(TargetMesh, MaterialIndex, Error);
	if (!MID)
	{
		return MakeErrorResult(TEXT(""), Error);
	}

	MID->SetTextureParameterValue(TextureParameterName, Texture);

	FRuntimeFileTextureResult Result;
	Result.bSuccess = true;
	Result.Type = ERuntimeFileTextureType::None;
	Result.Texture = Texture;
	return Result;
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
		return MakeErrorResult(FilePath, TEXT("TargetMesh is not set."));
	}

	if (TextureParameterName.IsNone())
	{
		return MakeErrorResult(FilePath, TEXT("TextureParameterName is not set."));
	}

	if (!RuntimeFileTextureInternal::IsSupportedFile(FilePath))
	{
		return MakeErrorResult(FilePath, FString::Printf(TEXT("Unsupported file extension: %s"), *FPaths::GetExtension(FilePath)));
	}

	if (RuntimeFileTextureInternal::IsSupportedImageFile(FilePath))
	{
		FString Error;
		UTexture2D* Texture = RuntimeFileTextureInternal::LoadImageTexture(TargetMesh, FilePath, Error);
		if (!Texture)
		{
			return MakeErrorResult(FilePath, Error);
		}

		FRuntimeFileTextureResult Result = ApplyTextureToMesh(TargetMesh, Texture, TextureParameterName, MaterialIndex);
		if (!Result.bSuccess)
		{
			Result.FilePath = FilePath;
			return Result;
		}

		Result.FilePath = FilePath;
		Result.Type = ERuntimeFileTextureType::Image;
		return Result;
	}

	UWorld* World = TargetMesh->GetWorld();
	if (!World)
	{
		return MakeErrorResult(FilePath, TEXT("TargetMesh world is not available."));
	}

	URuntimeFileTexturePlaybackSubsystem* PlaybackSubsystem = World->GetSubsystem<URuntimeFileTexturePlaybackSubsystem>();
	if (!PlaybackSubsystem)
	{
		return MakeErrorResult(FilePath, TEXT("Failed to acquire playback subsystem."));
	}

	URuntimeFileTexturePlaybackSession* Session = PlaybackSubsystem->GetOrCreateSession(TargetMesh);
	if (!Session)
	{
		return MakeErrorResult(FilePath, TEXT("Failed to create video playback session."));
	}

	FString VideoError;
	if (!Session->Initialize(TargetMesh, FilePath, bLoopVideo, VideoError))
	{
		PlaybackSubsystem->StopSession(TargetMesh);
		return MakeErrorResult(FilePath, VideoError);
	}

	UMediaTexture* MediaTexture = Session->GetMediaTexture();
	if (!MediaTexture)
	{
		PlaybackSubsystem->StopSession(TargetMesh);
		return MakeErrorResult(FilePath, TEXT("MediaTexture is not available."));
	}

	FRuntimeFileTextureResult TextureResult = ApplyTextureToMesh(TargetMesh, MediaTexture, TextureParameterName, MaterialIndex);
	if (!TextureResult.bSuccess)
	{
		PlaybackSubsystem->StopSession(TargetMesh);
		TextureResult.FilePath = FilePath;
		return TextureResult;
	}

	TextureResult.FilePath = FilePath;
	TextureResult.Type = ERuntimeFileTextureType::Video;
	return TextureResult;
}

void URuntimeFileTextureBPLibrary::StopVideoForMesh(UMeshComponent* TargetMesh)
{
	if (!TargetMesh)
	{
		return;
	}

	if (UWorld* World = TargetMesh->GetWorld())
	{
		if (URuntimeFileTexturePlaybackSubsystem* PlaybackSubsystem = World->GetSubsystem<URuntimeFileTexturePlaybackSubsystem>())
		{
			PlaybackSubsystem->StopSession(TargetMesh);
		}
	}
}
