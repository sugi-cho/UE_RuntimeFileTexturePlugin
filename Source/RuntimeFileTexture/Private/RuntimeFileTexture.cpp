#include "HAL/IConsoleManager.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "RuntimeFileTextureLog.h"
#include "RuntimeFileTextureSettings.h"

DEFINE_LOG_CATEGORY(LogRuntimeFileTexture);

namespace
{
	bool ApplyElectraDecoderSettings()
	{
#if PLATFORM_WINDOWS
		const URuntimeFileTextureSettings* Settings = GetDefault<URuntimeFileTextureSettings>();
		const int32 DisableD3D12Video = Settings->bEnableElectraD3D12HardwareDecoding ? 0 : 1;
		IConsoleVariable* DisableCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ElectraDecoders.bDisableD3D12Video"));
		IConsoleVariable* DoNotUseCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ElectraDecoders.bDoNotUseD3D12Video"));
		if (!DisableCVar || !DoNotUseCVar)
		{
			return false;
		}

		DisableCVar->Set(DisableD3D12Video, ECVF_SetByProjectSetting);
		DoNotUseCVar->Set(DisableD3D12Video, ECVF_SetByProjectSetting);
		UE_LOG(LogRuntimeFileTexture, Display,
			TEXT("FilePlay Electra D3D12 hardware decoding: %s [Disable=%d DoNotUse=%d]"),
			Settings->bEnableElectraD3D12HardwareDecoding ? TEXT("Enabled") : TEXT("Disabled"),
			DisableCVar->GetInt(), DoNotUseCVar->GetInt());
#endif
		return true;
	}
}

class FRuntimeFileTextureModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (!ApplyElectraDecoderSettings())
		{
			ModuleLoadingCompleteHandle = FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddLambda([]
			{
				ApplyElectraDecoderSettings();
			});
		}
	}

	virtual void ShutdownModule() override
	{
		if (ModuleLoadingCompleteHandle.IsValid())
		{
			FCoreDelegates::OnAllModuleLoadingPhasesComplete.Remove(ModuleLoadingCompleteHandle);
			ModuleLoadingCompleteHandle.Reset();
		}
	}

private:
	FDelegateHandle ModuleLoadingCompleteHandle;
};

IMPLEMENT_MODULE(FRuntimeFileTextureModule, RuntimeFileTexture)
