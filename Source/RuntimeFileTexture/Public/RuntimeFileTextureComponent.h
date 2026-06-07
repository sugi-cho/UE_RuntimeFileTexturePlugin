#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RuntimeFileTextureTypes.h"
#include "RuntimeFileTextureComponent.generated.h"

class UFileMediaSource;
class UMaterialInstanceDynamic;
class UMediaPlayer;
class UMediaTexture;
class AActor;
class UMeshComponent;
class UTexture2D;

UCLASS(ClassGroup=(Rendering), meta=(BlueprintSpawnableComponent))
class RUNTIMEFILETEXTURE_API URuntimeFileTextureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URuntimeFileTextureComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Runtime File Texture")
	TObjectPtr<AActor> TargetMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Runtime File Texture")
	FName TextureParameterName = TEXT("Texture");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Runtime File Texture")
	int32 MaterialIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Runtime File Texture")
	bool bLoopVideo = true;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RuntimeMID = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> StaticTexture = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> RuntimeMediaPlayer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> RuntimeMediaTexture = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFileMediaSource> RuntimeFileMediaSource = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Runtime File Texture|Debug")
	bool bLastOperationSucceeded = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Runtime File Texture|Debug")
	FString LastFilePath;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Runtime File Texture|Debug")
	ERuntimeFileTextureType LastType = ERuntimeFileTextureType::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Runtime File Texture|Debug")
	FString LastError;

	UFUNCTION(BlueprintCallable, Category="Runtime File Texture")
	FRuntimeFileTextureResult SelectFile();

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Runtime File Texture", meta=(DisplayName="Select File"))
	void SelectFileInEditor();

	UFUNCTION(BlueprintCallable, Category="Runtime File Texture")
	FRuntimeFileTextureResult ApplyFile(const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category="Runtime File Texture")
	void StopVideo();

private:
	UMeshComponent* ResolveTargetMesh(FString& OutError) const;
	FRuntimeFileTextureResult ApplyTextureToMesh(UTexture* Texture, ERuntimeFileTextureType Type, const FString& FilePath);
	FRuntimeFileTextureResult ApplyImageFile(const FString& FilePath);
	FRuntimeFileTextureResult ApplyVideoFile(const FString& FilePath);
	void ResetRuntimeMedia();
	UMaterialInstanceDynamic* GetOrCreateMID(FString& OutError);
	void CacheLastResult(const FRuntimeFileTextureResult& Result);
};
