#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RuntimeFileTextureSettings.generated.h"

/** Project-wide FilePlay settings. */
UCLASS(Config=Engine, DefaultConfig, meta=(DisplayName="FilePlay"))
class RUNTIMEFILETEXTURE_API URuntimeFileTextureSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("FilePlay"); }

	/**
	 * Enables Electra's native D3D12 video decoder on Windows.
	 * A restart is required because the decoder module is initialized during engine startup.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Electra", meta=(ConfigRestartRequired=true))
	bool bEnableElectraD3D12HardwareDecoding = true;
};
