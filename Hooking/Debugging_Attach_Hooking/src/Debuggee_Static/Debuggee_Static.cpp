#include <stdio.h>
#include <windows.h>

void Msg() {
	MessageBoxW(NULL, L"MessageBoxW", L"TEST", MB_OK | MB_ICONASTERISK);
	MessageBoxA(NULL, "MessageBoxA", "TEST", MB_OK | MB_ICONASTERISK);
} // 테스트 함수 (정적 호출)

int main() {
#ifdef _WIN64
	printf("[Debuggee x64 // Static Library]\n");
#else
	printf("[Debuggee x86 // Static Library]\n");
#endif
	printf("PID : %d\n",GetCurrentProcessId());
	Msg();
	system("pause");
	Msg();
	Msg();
	system("pause");
}
