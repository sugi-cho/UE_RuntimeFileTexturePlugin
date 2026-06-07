#include "RuntimeFileTextureInternal.h"

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"
#include <shobjidl.h>
#include "Windows/HideWindowsPlatformTypes.h"

namespace
{
	bool GetDialogFilePath(IFileOpenDialog* Dialog, FString& OutFilePath)
	{
		IShellItem* Item = nullptr;
		const HRESULT Hr = Dialog->GetResult(&Item);
		if (FAILED(Hr) || Item == nullptr)
		{
			return false;
		}

		PWSTR Path = nullptr;
		const HRESULT PathHr = Item->GetDisplayName(SIGDN_FILESYSPATH, &Path);
		Item->Release();
		if (FAILED(PathHr) || Path == nullptr)
		{
			return false;
		}

		OutFilePath = FString(Path);
		CoTaskMemFree(Path);
		return true;
	}
}

bool RuntimeFileTextureInternal::SelectFileDialog(FString& OutFilePath)
{
	OutFilePath.Reset();

	HRESULT CoInitResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	const bool bShouldUninitialize = SUCCEEDED(CoInitResult);
	if (FAILED(CoInitResult) && CoInitResult != RPC_E_CHANGED_MODE)
	{
		return false;
	}

	IFileOpenDialog* Dialog = nullptr;
	const HRESULT CreateHr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Dialog));
	if (FAILED(CreateHr) || Dialog == nullptr)
	{
		if (bShouldUninitialize)
		{
			CoUninitialize();
		}
		return false;
	}

	const DWORD Options = FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_STRICTFILETYPES;
	Dialog->SetOptions(Options & ~FOS_STRICTFILETYPES);

	static const COMDLG_FILTERSPEC Filters[] =
	{
		{ L"Image and Video Files (*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.exr;*.mp4;*.mov;*.wmv;*.avi;*.m4v)", L"*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.exr;*.mp4;*.mov;*.wmv;*.avi;*.m4v" },
		{ L"Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.exr)", L"*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.exr" },
		{ L"Video Files (*.mp4;*.mov;*.wmv;*.avi;*.m4v)", L"*.mp4;*.mov;*.wmv;*.avi;*.m4v" },
		{ L"All Files (*.*)", L"*.*" }
	};
	Dialog->SetFileTypes(UE_ARRAY_COUNT(Filters), Filters);
	Dialog->SetFileTypeIndex(1);
	Dialog->SetTitle(L"Select Image or Video File");

	const HRESULT ShowHr = Dialog->Show(nullptr);
	const bool bSuccess = SUCCEEDED(ShowHr) && GetDialogFilePath(Dialog, OutFilePath);

	Dialog->Release();
	if (bShouldUninitialize)
	{
		CoUninitialize();
	}

	return bSuccess;
}

#else

bool RuntimeFileTextureInternal::SelectFileDialog(FString& OutFilePath)
{
	OutFilePath.Reset();
	return false;
}

#endif
