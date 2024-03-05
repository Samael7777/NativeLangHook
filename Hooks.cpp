// ReSharper disable IdentifierTypo
// ReSharper disable CppInconsistentNaming

// ReSharper disable CppClangTidyPerformanceNoIntToPtr
#pragma comment(linker, "/MERGE:.pdata=.data")
#pragma comment(linker, "/MERGE:.rdata=.data")
//#pragma comment(linker, "/SECTION:.text,EWR") //Sample!

#include "Hooks.h"

#include <imm.h>

unsigned int LayoutChangedMessage = 0;
unsigned int LayoutChangeRequestMessage = 0;
unsigned int ExitHookRequestMessage = 0;

HMODULE ModuleHandle = nullptr;
HANDLE MapObject = nullptr;
SharedData* Shared = nullptr;

const wchar_t* HexDigits = L"0123456789abcdef";
wchar_t Buffer[9] = {0,0,0,0,0,0,0,0,0};

LRESULT CALLBACK HookProc(int code, WPARAM w_param, LPARAM l_param);

void IntToHexBuffer(unsigned int number, wchar_t* buffer);
void UnloadInjectedLib();

namespace 
{
	bool CreateSharedDataMapping()
	{
		MapObject = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			nullptr,
			PAGE_READWRITE,
			0,
			sizeof(SharedData),
			MemMapFilename);
		if (MapObject == nullptr) return false;
		Shared = static_cast<SharedData*>(MapViewOfFile(
			MapObject,
			FILE_MAP_WRITE,
			0,
			0,
			0));
		if (Shared == nullptr) return false;
		return true;
	}

	void CloseMappedFiles()
	{
		if (Shared != nullptr)
		{
			UnmapViewOfFile(Shared);
			Shared = nullptr;
		}
		if (MapObject != nullptr)
		{
			CloseHandle(MapObject);
			MapObject = nullptr;
		}
	}

	void DestroyHookForCreatorThread()
	{
		if (Shared->CreatorThreadId == GetCurrentThreadId()) 
		{
			DestroyLangHook();
		}
	}
}

unsigned int GetLayoutChangedMessageCode()
{
	return LayoutChangedMessage;
}

unsigned int GetLayoutChangeRequestMessageCode()
{
	return LayoutChangeRequestMessage;
}

HHOOK SetLangHook(const HWND captureWindowHandle)
{
	Shared->CaptureWindow = captureWindowHandle;

	if (Shared->LangHook != nullptr)
	{
		return Shared->LangHook;
	}

	auto const foregorundWindow = GetForegroundWindow();
	auto const foregroundWndProcId = GetWindowThreadProcessId(foregorundWindow, nullptr);
	Shared->CurrentLayout = GetKeyboardLayout(foregroundWndProcId);
	Shared->CreatorThreadId = GetCurrentThreadId();
	Shared->LangHook = SetWindowsHookEx(WH_CALLWNDPROC, HookProc, ModuleHandle, 0);
	return Shared->LangHook;
}

LRESULT CALLBACK HookProc(const int code, const WPARAM w_param, const LPARAM l_param)
{
	if (code == HC_ACTION)
	{
		auto const pMsg = reinterpret_cast<tagCWPSTRUCT*>(l_param);

		if (pMsg->message == WM_INPUTLANGCHANGE
			&& reinterpret_cast<HKL>(pMsg->lParam) != Shared->CurrentLayout)
		{
			Shared->CurrentLayout = reinterpret_cast<HKL>(pMsg->lParam);
			if (Shared->CaptureWindow != nullptr)
			{
				PostMessage(Shared->CaptureWindow, LayoutChangedMessage, pMsg->wParam, pMsg->lParam);
			}
			else
			{
				PostThreadMessage(Shared->CreatorThreadId, LayoutChangedMessage, pMsg->wParam, pMsg->lParam);
			}
		}

		if(pMsg->message == LayoutChangeRequestMessage)
		{
			if(pMsg->lParam != 0 || pMsg->wParam != 0)
			{
				const auto hkl = reinterpret_cast<HKL>(pMsg->lParam);
				const auto klid = static_cast<unsigned int>(pMsg->wParam);
				const auto imeWndHandle = ImmGetDefaultIMEWnd(nullptr);
				const auto imeContext = ImmGetContext(imeWndHandle);

				if (imeContext != nullptr)
				{
					ImmSetOpenStatus(imeContext, false);
					if (hkl != nullptr) ActivateKeyboardLayout(hkl, 1);
					if (klid != 0)
					{
						IntToHexBuffer(klid, Buffer);
						LoadKeyboardLayoutW(Buffer, 1);
					}
					ImmSetOpenStatus(imeContext, true);
					ImmReleaseContext(imeWndHandle, imeContext);
				}
			}
		}

		/*if(pMsg->message == ExitHookRequestMessage)
		{
			FreeLibrary(ModuleHandle);
		}*/
	}
	return CallNextHookEx(Shared->LangHook, code, w_param, l_param);
}

bool DestroyLangHook()
{
	if (Shared->LangHook == nullptr) return false;

	//SendMessage(reinterpret_cast<HWND>(0xFFFF), ExitHookRequestMessage, 0, 0);
	auto const result = UnhookWindowsHookEx(Shared->LangHook);
	Shared->LangHook = nullptr;
	CloseMappedFiles();
	return result;
}


inline void IntToHexBuffer(const UINT32 number, wchar_t* buffer)
{
	for (int i = 0; i <= 7; i++)
	{
		buffer[i] = '0';
	}

	int index = 7;
	UINT32 temp = number;
	while(temp != 0)
	{
		const UINT32 modulo = temp % 16;
		buffer[index] = HexDigits[modulo];
		temp = (temp - modulo) / 16;
		index --;
	}
}


BOOL APIENTRY DllMain(const HMODULE hModule,
                      const DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		LayoutChangedMessage = RegisterWindowMessage(LayoutChangedMessageName);
		LayoutChangeRequestMessage = RegisterWindowMessage(LayoutChangeRequestMessageName);
		ExitHookRequestMessage = RegisterWindowMessage(ExitHookRequestMessageName);
		ModuleHandle = hModule;
		CreateSharedDataMapping();
		break;
	case DLL_PROCESS_DETACH:
		DestroyHookForCreatorThread();
		CloseMappedFiles();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	default:
		break;
	}

	return TRUE;
}