#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RuntimeFileTexturePlaybackSubsystem.generated.h"

class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;
class UMeshComponent;

UCLASS()
class RUNTIMEFILETEXTURE_API URuntimeFileTexturePlaybackSession : public UObject
{
	GENERATED_BODY()

public:
	bool Initialize(UMeshComponent* InTargetMesh, const FString& FilePath, bool bLoopVideo, FString& OutError);
	void Stop();

	UMediaTexture* GetMediaTexture() const { return RuntimeMediaTexture; }
	const UMeshComponent* GetTargetMesh() const { return TargetMesh.Get(); }

private:
	UFUNCTION()
	void HandleEndReached();

	TWeakObjectPtr<UMeshComponent> TargetMesh;
	bool bLooping = true;

	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> RuntimeMediaPlayer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> RuntimeMediaTexture = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFileMediaSource> RuntimeFileMediaSource = nullptr;
};

UCLASS()
class RUNTIMEFILETEXTURE_API URuntimeFileTexturePlaybackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	URuntimeFileTexturePlaybackSession* GetOrCreateSession(UMeshComponent* TargetMesh);
	void StopSession(UMeshComponent* TargetMesh);

protected:
	virtual void Deinitialize() override;

private:
	void CleanupInvalidSessions();
	URuntimeFileTexturePlaybackSession* FindSession(const UMeshComponent* TargetMesh) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URuntimeFileTexturePlaybackSession>> ActiveSessions;
};
