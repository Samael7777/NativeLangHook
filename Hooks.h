#pragma once
#include <Windows.h>

#define DLL_EXPORT __declspec(dllexport)
const LPCTSTR LayoutChangedMessageName = reinterpret_cast<LPCTSTR>("LangHookLayoutChanged_{DD57B1F7}");
const LPCTSTR LayoutChangeRequestMessageName = reinterpret_cast<LPCTSTR>("LangHookLayoutChangeRequest_{D5715F6B}");
const LPCTSTR ExitHookRequestMessageName = reinterpret_cast<LPCTSTR>("LangHookExitHookRequest_{D5715F6B}");

#ifdef _M_X64
const LPCTSTR MemMapFilename = reinterpret_cast<LPCTSTR>("DllSharedMemory{58ABC9EE}_X64");
#else
const LPCTSTR MemMapFilename = reinterpret_cast<LPCTSTR>("DllSharedMemory{58ABC9EE}_X86");
#endif


struct SharedData
{
	HWND CaptureWindow;
	HHOOK LangHook;
	DWORD CreatorThreadId;
	HKL CurrentLayout;
};

#ifdef __cplusplus
extern "C"
{
#endif

	DLL_EXPORT unsigned int GetLayoutChangedMessageCode();
	DLL_EXPORT unsigned int GetLayoutChangeRequestMessageCode();
	DLL_EXPORT HHOOK SetLangHook(HWND captureWindowHandle);
	DLL_EXPORT bool DestroyLangHook();

#ifdef __cplusplus
}
#endif