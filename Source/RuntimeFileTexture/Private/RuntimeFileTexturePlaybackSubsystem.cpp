#include "RuntimeFileTexturePlaybackSubsystem.h"

#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Components/MeshComponent.h"
#include "Misc/Paths.h"
#include "Misc/Timespan.h"

bool URuntimeFileTexturePlaybackSession::Initialize(UMeshComponent* InTargetMesh, const FString& FilePath, bool bLoopVideo, FString& OutError)
{
	OutError.Reset();
	Stop();
	TargetMesh = InTargetMesh;
	bLooping = bLoopVideo;

	if (!TargetMesh.IsValid())
	{
		OutError = TEXT("TargetMesh is not set.");
		return false;
	}

	RuntimeMediaPlayer = NewObject<UMediaPlayer>(this);
	RuntimeMediaTexture = NewObject<UMediaTexture>(this);
	RuntimeFileMediaSource = NewObject<UFileMediaSource>(this);
	if (!RuntimeMediaPlayer || !RuntimeMediaTexture || !RuntimeFileMediaSource)
	{
		OutError = TEXT("Failed to open video media source.");
		Stop();
		return false;
	}

	RuntimeFileMediaSource->SetFilePath(FilePath);
	RuntimeMediaPlayer->SetLooping(bLooping);
	RuntimeMediaPlayer->OnEndReached.AddDynamic(this, &URuntimeFileTexturePlaybackSession::HandleEndReached);
	RuntimeMediaTexture->SetMediaPlayer(RuntimeMediaPlayer);
	RuntimeMediaTexture->UpdateResource();

	if (!RuntimeMediaPlayer->OpenSource(RuntimeFileMediaSource))
	{
		OutError = TEXT("Failed to open video media source.");
		Stop();
		return false;
	}

	RuntimeMediaPlayer->Play();
	return true;
}

void URuntimeFileTexturePlaybackSession::Stop()
{
	if (RuntimeMediaPlayer)
	{
		RuntimeMediaPlayer->OnEndReached.RemoveDynamic(this, &URuntimeFileTexturePlaybackSession::HandleEndReached);
		RuntimeMediaPlayer->Close();
	}
	if (RuntimeMediaTexture)
	{
		RuntimeMediaTexture->SetMediaPlayer(nullptr);
	}

	RuntimeMediaPlayer = nullptr;
	RuntimeMediaTexture = nullptr;
	RuntimeFileMediaSource = nullptr;
	TargetMesh.Reset();
	bLooping = true;
}

void URuntimeFileTexturePlaybackSession::HandleEndReached()
{
	if (!bLooping || !RuntimeMediaPlayer)
	{
		return;
	}

	RuntimeMediaPlayer->Seek(FTimespan::Zero());
	RuntimeMediaPlayer->Play();
}

URuntimeFileTexturePlaybackSession* URuntimeFileTexturePlaybackSubsystem::FindSession(const UMeshComponent* TargetMesh) const
{
	for (URuntimeFileTexturePlaybackSession* Session : ActiveSessions)
	{
		if (Session)
		{
			if (Session->GetTargetMesh() == TargetMesh)
			{
				return Session;
			}
		}
	}

	return nullptr;
}

void URuntimeFileTexturePlaybackSubsystem::CleanupInvalidSessions()
{
	ActiveSessions.RemoveAll([](const TObjectPtr<URuntimeFileTexturePlaybackSession>& Session)
	{
		return !Session || !Session->GetTargetMesh();
	});
}

URuntimeFileTexturePlaybackSession* URuntimeFileTexturePlaybackSubsystem::GetOrCreateSession(UMeshComponent* TargetMesh)
{
	if (!TargetMesh)
	{
		return nullptr;
	}

	CleanupInvalidSessions();

	if (URuntimeFileTexturePlaybackSession* ExistingSession = FindSession(TargetMesh))
	{
		return ExistingSession;
	}

	URuntimeFileTexturePlaybackSession* NewSession = NewObject<URuntimeFileTexturePlaybackSession>(this);
	if (!NewSession)
	{
		return nullptr;
	}

	ActiveSessions.Add(NewSession);
	return NewSession;
}

void URuntimeFileTexturePlaybackSubsystem::StopSession(UMeshComponent* TargetMesh)
{
	if (!TargetMesh)
	{
		return;
	}

	for (int32 Index = ActiveSessions.Num() - 1; Index >= 0; --Index)
	{
		URuntimeFileTexturePlaybackSession* Session = ActiveSessions[Index];
		if (!Session || Session->GetTargetMesh() != TargetMesh)
		{
			continue;
		}

		Session->Stop();
		ActiveSessions.RemoveAtSwap(Index);
	}
}

void URuntimeFileTexturePlaybackSubsystem::Deinitialize()
{
	for (URuntimeFileTexturePlaybackSession* Session : ActiveSessions)
	{
		if (Session)
		{
			Session->Stop();
		}
	}

	ActiveSessions.Empty();
	Super::Deinitialize();
}
