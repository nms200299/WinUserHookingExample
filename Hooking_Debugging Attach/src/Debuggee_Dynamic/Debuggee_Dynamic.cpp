#include <stdio.h>
#include <windows.h>

typedef int (WINAPI* PFMESSAGEBOXW)(
	HWND     hWnd,
	LPCWSTR  lpText,
	LPCWSTR  lpCaption,
	UINT     uType
	);

typedef int (WINAPI* PFMESSAGEBOXA)(
	HWND     hWnd,
	LPCSTR  lpText,
	LPCSTR  lpCaption,
	UINT     uType
	);

HMODULE hMod;
PFMESSAGEBOXW pMessageBoxW;
PFMESSAGEBOXA pMessageBoxA;

void Msg() {
	if (!hMod) return;
	pMessageBoxW(NULL, L"MessageBoxW", L"TEST", MB_OK | MB_ICONASTERISK);
	pMessageBoxA(NULL, "MessageBoxA", "TEST", MB_OK | MB_ICONASTERISK);
} // 테스트 함수 (동적 호출)

int main() {
#ifdef _WIN64
	printf("[Debuggee x64 // Dynamic Library]\n");
#else
	printf("[Debuggee x86 // Dynamic Library]\n");
#endif
	printf("PID : %d\n", GetCurrentProcessId());
	system("pause");
	hMod = LoadLibrary(L"user32.dll");
	if (!hMod) return -1;
	pMessageBoxW = (PFMESSAGEBOXW)GetProcAddress(hMod, "MessageBoxW");
	pMessageBoxA = (PFMESSAGEBOXA)GetProcAddress(hMod, "MessageBoxA");
	Msg();
	Msg();
	system("pause");
	if (hMod) FreeLibrary(hMod);
}
