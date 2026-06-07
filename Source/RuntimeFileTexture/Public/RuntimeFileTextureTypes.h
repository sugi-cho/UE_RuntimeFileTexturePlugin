#pragma once

#include "CoreMinimal.h"
#include "RuntimeFileTextureTypes.generated.h"

class UTexture;

UENUM(BlueprintType)
enum class ERuntimeFileTextureType : uint8
{
	None,
	Image,
	Video
};

USTRUCT(BlueprintType)
struct FRuntimeFileTextureResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly)
	FString FilePath;

	UPROPERTY(BlueprintReadOnly)
	ERuntimeFileTextureType Type = ERuntimeFileTextureType::None;

	UPROPERTY(BlueprintReadOnly)
	UTexture* Texture = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FString Error;
};
