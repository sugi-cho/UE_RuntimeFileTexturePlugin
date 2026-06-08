#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RuntimeFileTextureTypes.h"
#include "RuntimeFileTextureBPLibrary.generated.h"

class UMeshComponent;
class UTexture;

UCLASS()
class RUNTIMEFILETEXTURE_API URuntimeFileTextureBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Runtime File Texture")
	static bool SelectFile(FString& OutFilePath);

	UFUNCTION(BlueprintCallable, Category="Runtime File Texture")
	static FRuntimeFileTextureResult ApplyTextureToMesh(
		UMeshComponent* TargetMesh,
		UTexture* Texture,
		FName TextureParameterName,
		int32 MaterialIndex = 0
	);

	UFUNCTION(BlueprintCallable, Category="Runtime File Texture")
	static FRuntimeFileTextureResult ApplyFileToMesh(
		UMeshComponent* TargetMesh,
		FName TextureParameterName,
		const FString& FilePath,
		int32 MaterialIndex = 0,
		bool bLoopVideo = true
	);
};
