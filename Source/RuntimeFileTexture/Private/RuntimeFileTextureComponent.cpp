#include "RuntimeFileTextureComponent.h"

#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Misc/Paths.h"
#include "RuntimeFileTextureInternal.h"

URuntimeFileTextureComponent::URuntimeFileTextureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FRuntimeFileTextureResult URuntimeFileTextureComponent::SelectFile()
{
	FString FilePath;
	if (!RuntimeFileTextureInternal::SelectFileDialog(FilePath))
	{
		FRuntimeFileTextureResult Result;
		Result.bSuccess = false;
		Result.Type = ERuntimeFileTextureType::None;
		Result.Texture = nullptr;
		Result.Error = TEXT("File selection was cancelled or failed.");
		CacheLastResult(Result);
		return Result;
	}

	FRuntimeFileTextureResult Result = ApplyFile(FilePath);
	CacheLastResult(Result);
	return Result;
}

void URuntimeFileTextureComponent::SelectFileInEditor()
{
	CacheLastResult(SelectFile());
}

UMeshComponent* URuntimeFileTextureComponent::ResolveTargetMesh(FString& OutError) const
{
	OutError.Reset();

	if (!TargetMesh)
	{
		OutError = TEXT("TargetMesh is not set.");
		return nullptr;
	}

	if (const AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(TargetMesh))
	{
		return StaticMeshActor->GetStaticMeshComponent();
	}

	if (UMeshComponent* FoundMeshComponent = TargetMesh->FindComponentByClass<UMeshComponent>())
	{
		return FoundMeshComponent;
	}

	OutError = TEXT("TargetMesh actor does not contain a mesh component.");
	return nullptr;
}

FRuntimeFileTextureResult URuntimeFileTextureComponent::ApplyFile(const FString& FilePath)
{
	FString TargetMeshError;
	if (!ResolveTargetMesh(TargetMeshError))
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TargetMeshError;
		CacheLastResult(Result);
		return Result;
	}

	if (TextureParameterName.IsNone())
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("TextureParameterName is not set.");
		CacheLastResult(Result);
		return Result;
	}

	if (!RuntimeFileTextureInternal::IsSupportedFile(FilePath))
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = FString::Printf(TEXT("Unsupported file extension: %s"), *FPaths::GetExtension(FilePath));
		CacheLastResult(Result);
		return Result;
	}

	FRuntimeFileTextureResult Result = RuntimeFileTextureInternal::IsSupportedImageFile(FilePath) ? ApplyImageFile(FilePath) : ApplyVideoFile(FilePath);
	CacheLastResult(Result);
	return Result;
}

void URuntimeFileTextureComponent::StopVideo()
{
	if (RuntimeMediaPlayer)
	{
		RuntimeMediaPlayer->Close();
		RuntimeMediaPlayer = nullptr;
	}
	if (RuntimeMediaTexture)
	{
		RuntimeMediaTexture->SetMediaPlayer(nullptr);
	}
	RuntimeMediaTexture = nullptr;
	RuntimeFileMediaSource = nullptr;
}

void URuntimeFileTextureComponent::ResetRuntimeMedia()
{
	StopVideo();
}

UMaterialInstanceDynamic* URuntimeFileTextureComponent::GetOrCreateMID(FString& OutError)
{
	OutError.Reset();

	UMeshComponent* MeshComponent = ResolveTargetMesh(OutError);
	if (!MeshComponent)
	{
		return nullptr;
	}

	if (RuntimeMID)
	{
		if (MeshComponent->GetMaterial(MaterialIndex) != RuntimeMID)
		{
			MeshComponent->SetMaterial(MaterialIndex, RuntimeMID);
		}

		return RuntimeMID;
	}

	if (UMaterialInstanceDynamic* ExistingMID = Cast<UMaterialInstanceDynamic>(MeshComponent->GetMaterial(MaterialIndex)))
	{
		RuntimeMID = ExistingMID;
		return RuntimeMID;
	}

	UMaterialInterface* BaseMaterial = MeshComponent->GetMaterial(MaterialIndex);
	if (!BaseMaterial)
	{
		OutError = TEXT("Failed to create dynamic material instance.");
		return nullptr;
	}

	RuntimeMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (!RuntimeMID)
	{
		OutError = TEXT("Failed to create dynamic material instance.");
		return nullptr;
	}

	MeshComponent->SetMaterial(MaterialIndex, RuntimeMID);
	return RuntimeMID;
}

FRuntimeFileTextureResult URuntimeFileTextureComponent::ApplyTextureToMesh(UTexture* Texture, ERuntimeFileTextureType Type, const FString& FilePath)
{
	if (!Texture)
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("Texture is not set.");
		CacheLastResult(Result);
		return Result;
	}

	FString Error;
	UMaterialInstanceDynamic* MID = GetOrCreateMID(Error);
	if (!MID)
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = Error;
		CacheLastResult(Result);
		return Result;
	}

	MID->SetTextureParameterValue(TextureParameterName, Texture);
	FRuntimeFileTextureResult Result;
	Result.bSuccess = true;
	Result.FilePath = FilePath;
	Result.Type = Type;
	Result.Texture = Texture;
	return Result;
}

FRuntimeFileTextureResult URuntimeFileTextureComponent::ApplyImageFile(const FString& FilePath)
{
	ResetRuntimeMedia();

	FString Error;
	StaticTexture = RuntimeFileTextureInternal::LoadImageTexture(this, FilePath, Error);
	if (!StaticTexture)
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = Error;
		CacheLastResult(Result);
		return Result;
	}

	return ApplyTextureToMesh(StaticTexture, ERuntimeFileTextureType::Image, FilePath);
}

FRuntimeFileTextureResult URuntimeFileTextureComponent::ApplyVideoFile(const FString& FilePath)
{
	ResetRuntimeMedia();

	RuntimeMediaPlayer = NewObject<UMediaPlayer>(this);
	RuntimeMediaTexture = NewObject<UMediaTexture>(this);
	RuntimeFileMediaSource = NewObject<UFileMediaSource>(this);
	if (!RuntimeMediaPlayer || !RuntimeMediaTexture || !RuntimeFileMediaSource)
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("Failed to open video media source.");
		CacheLastResult(Result);
		return Result;
	}

	RuntimeFileMediaSource->SetFilePath(FilePath);
	RuntimeMediaPlayer->SetLooping(bLoopVideo);
	RuntimeMediaTexture->SetMediaPlayer(RuntimeMediaPlayer);
	RuntimeMediaTexture->UpdateResource();

	const FRuntimeFileTextureResult TextureResult = ApplyTextureToMesh(RuntimeMediaTexture, ERuntimeFileTextureType::Video, FilePath);
	if (!TextureResult.bSuccess)
	{
		CacheLastResult(TextureResult);
		return TextureResult;
	}

	if (!RuntimeMediaPlayer->OpenSource(RuntimeFileMediaSource))
	{
		FRuntimeFileTextureResult Result;
		Result.FilePath = FilePath;
		Result.Error = TEXT("Failed to open video media source.");
		CacheLastResult(Result);
		return Result;
	}

	RuntimeMediaPlayer->Play();
	CacheLastResult(TextureResult);
	return TextureResult;
}

void URuntimeFileTextureComponent::CacheLastResult(const FRuntimeFileTextureResult& Result)
{
	bLastOperationSucceeded = Result.bSuccess;
	LastFilePath = Result.FilePath;
	LastType = Result.Type;
	LastError = Result.Error;
}
