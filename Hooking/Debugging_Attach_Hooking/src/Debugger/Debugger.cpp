/*
Title : Debugg 프로그램
Summary : 후킹을 수행하는 디버거 프로그램입니다.
*/

#include <stdio.h>
#include <windows.h>
#include <process.h> // _beginthreadex
#include <psapi.h> // EnumProcessModules
#include <Shlwapi.h> // StrStrIW
#pragma comment(lib, "Shlwapi.lib")



typedef struct _API_STRUCT {
	LPVOID pfAddress;
	HMODULE hModule;
	char szModuleName[MAX_PATH];
	char szFuncName[MAX_PATH];
	BYTE chOrgByte;
	_API_STRUCT* NextApiStruct;
} API_STRUCT;

API_STRUCT g_MessageBoxW = { NULL, NULL, "USER32.dll" , "MessageBoxW", 0x00, NULL };
API_STRUCT g_MessageBoxA = { NULL, NULL, "USER32.dll" , "MessageBoxA", 0x00, &g_MessageBoxW };


// 프로세스에서 특정 모듈이 로드되었는지 확인합니다.
HMODULE IsUsingModule(HANDLE hProcess, char szFindMod[]) {
	HMODULE hMods[1024];
	DWORD cbNeeded;
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)){
		int count = cbNeeded / sizeof(HMODULE);
		for (int i = 0; i < count; i++){
			char szModule[MAX_PATH];
			GetModuleBaseNameA(
				hProcess,
				hMods[i],
				szModule,
				MAX_PATH);
			if (_stricmp(szModule, szFindMod) == 0) {
				return hMods[i];
				break;
			}
		}
	}
	return NULL;
}

// 브레이크 포인트 설치
bool SetupBP(HANDLE hProcess, API_STRUCT* ApiStruct) {
	if (!ApiStruct->pfAddress) return FALSE;
	DWORD oldProtect;
	VirtualProtectEx(hProcess, ApiStruct->pfAddress, sizeof(BYTE), PAGE_READWRITE, &oldProtect);
	// 가상 메모리 공간 속성을 READWRITE로 변경

	BYTE chOrgByte, chINT3 = 0xCC;
	BOOL bReadRes = ReadProcessMemory(hProcess, ApiStruct->pfAddress, &chOrgByte, sizeof(BYTE), NULL);
	// 함수 시작 주소 1바이트를 읽어 백업
	if (chOrgByte == chINT3) {
		VirtualProtectEx(hProcess, ApiStruct->pfAddress, sizeof(BYTE), oldProtect, &oldProtect);
		return TRUE;
	} // 만약 이미 BP가 설치되어 있으면 재설치 하지 않음.

	ApiStruct->chOrgByte = chOrgByte;
	BOOL bWriteRes = WriteProcessMemory(hProcess, ApiStruct->pfAddress, &chINT3, sizeof(BYTE), NULL);
	// BP 설치
	VirtualProtectEx(hProcess, ApiStruct->pfAddress, sizeof(BYTE), oldProtect, &oldProtect);
	// 가상 메모리 공간 속성 복원
	FlushInstructionCache(hProcess, ApiStruct->pfAddress, sizeof(BYTE));
	// I-Cache 초기화
	return (bReadRes && bWriteRes);
} 

// 브레이크 포인트 제거
bool RemoveBP(HANDLE hProcess, API_STRUCT* ApiStruct) {
	if (!ApiStruct->pfAddress) return FALSE;
	DWORD oldProtect;
	VirtualProtectEx(hProcess, ApiStruct->pfAddress, sizeof(BYTE), PAGE_READWRITE, &oldProtect);
	// 가상 메모리 공간 속성을 READWRITE로 변경
	BOOL bWriteRes = WriteProcessMemory(hProcess, ApiStruct->pfAddress, &ApiStruct->chOrgByte, sizeof(BYTE), NULL);
	// 백업된 바이트를 메모리에 써서 복원
	VirtualProtectEx(hProcess, ApiStruct->pfAddress, sizeof(BYTE), oldProtect, &oldProtect);
	// 가상 메모리 공간 속성 복원
	FlushInstructionCache(hProcess, ApiStruct->pfAddress, sizeof(BYTE));
	// I-Cache 초기화
	return (bWriteRes);
}


void DebugEventSetBP(HANDLE hProcess, LPDEBUG_EVENT pde, DWORD dwPID, DWORD dwTID, BOOL IsDynamic) {
	API_STRUCT* CurrentAPI = &g_MessageBoxA;
	LOAD_DLL_DEBUG_INFO DebugInfo = {0,};

	while (CurrentAPI) {
		if (IsDynamic) {
		// DLL 동적 링크 일 때
			DebugInfo = pde->u.LoadDll;
			char szModulePath[MAX_PATH] = {0,};
			if (DebugInfo.hFile) {
				GetFinalPathNameByHandleA(DebugInfo.hFile, szModulePath, MAX_PATH, FILE_NAME_NORMALIZED);
			} // 로드하려는 모듈 경로를 알아냄

			if (!StrStrIA(szModulePath, CurrentAPI->szModuleName)) {
				CurrentAPI = CurrentAPI->NextApiStruct;
				continue;
			} // 해당 경로에 후킹 대상 모듈이 포함되어 있는지 확인

			CurrentAPI->hModule = (HMODULE)DebugInfo.lpBaseOfDll;
			printf("[I][%lu][%lu] %s : LOAD_DLL_DEBUG_EVENT (%s)\n",
				dwPID, dwTID, __func__, CurrentAPI->szModuleName);
		} else {
		// DLL 정적 링크일 때,
			CurrentAPI->hModule = IsUsingModule(hProcess, CurrentAPI->szModuleName);
			// 후킹 대상 모듈이 현재 디버기에서 로드 되어 있는지 확인
		}

		if (CurrentAPI->hModule) {
			BOOL bLoaded = FALSE;
			HMODULE hModule = GetModuleHandleA(CurrentAPI->szModuleName);
			if (!hModule) {
				hModule = LoadLibraryA(CurrentAPI->szModuleName);
				bLoaded = TRUE;
			}
			if (!hModule) return;
			FARPROC pApiAddr = GetProcAddress(hModule, CurrentAPI->szFuncName);
			SIZE_T rva = (BYTE*)pApiAddr - (BYTE*)hModule;
			if (bLoaded) FreeLibrary(hModule);
			// 디버거에도 해당 모듈을 로드하여 함수의 RVA를 계산

			CurrentAPI->pfAddress = (BYTE*)CurrentAPI->hModule + rva;
			// RVA를 통해 디버기 함수 주소를 계산

			if (SetupBP(hProcess, CurrentAPI)) {
			// 함수 시작 주소에 브레이크 포인트를 설치
				printf("[I][%lu][%lu] %s : %s %s (0x%p) -> INT3 Setup Success\n",
					dwPID, dwTID, __func__,
					CurrentAPI->szModuleName, CurrentAPI->szFuncName, CurrentAPI->pfAddress);
			} else {
				printf("[F][%lu][%lu] %s : %s %s (0x%p) -> INT3 Setup Fail..\n",
					dwPID, dwTID, __func__,
					CurrentAPI->szModuleName, CurrentAPI->szFuncName, CurrentAPI->pfAddress);
			}
		}
		CurrentAPI = CurrentAPI->NextApiStruct;
	}

	if (DebugInfo.hFile) CloseHandle(DebugInfo.hFile);
	// 동적 링크시, 파일 핸들을 닫아줘야 함.
	return;
}

API_STRUCT* gTrapApiStruct = NULL;

void DebugEventException(HANDLE hProcess, LPDEBUG_EVENT pde, DWORD dwPID, DWORD dwTID) {
	PEXCEPTION_RECORD per = &pde->u.Exception.ExceptionRecord;

	switch (per->ExceptionCode) {
		case EXCEPTION_BREAKPOINT: {
		// 브레이크 포인트(INT3) 예외인 경우
			API_STRUCT* CurrentAPI = &g_MessageBoxA;
			while (CurrentAPI) {
				if (CurrentAPI->pfAddress != (LPVOID)per->ExceptionAddress) {
					CurrentAPI = CurrentAPI->NextApiStruct;
					continue;
				} // 예외 주소와 함수 주소가 일치하는지 확인

				gTrapApiStruct = CurrentAPI;
				printf("[I][%lu][%lu] %s : %s %s (0x%p) : INT3 Break Enter\n",
					dwPID, dwTID, __func__,
					CurrentAPI->szModuleName, CurrentAPI->szFuncName, CurrentAPI->pfAddress);
				RemoveBP(hProcess, CurrentAPI);
				// 브레이크 포인트 제거

				HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, pde->dwThreadId);
				// 예외가 발생한 TID의 스레드 핸들을 구함
				LPVOID pParm2Addr = NULL;
				LPVOID pParm3Addr = NULL;
				CONTEXT ctx;
				ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
				GetThreadContext(hThread, &ctx);
				// 스레드 켄텍스트를 읽어옴
				ctx.EFlags |= 0x100;
				// Single Step을 위해 Trap Flag 설정
#ifdef _WIN64
				ctx.Rip = (DWORD64)CurrentAPI->pfAddress;
				// 0xCC가 실행된 상태임으로 증가한 RIP를 함수 주소로 맞춰줌
				SetThreadContext(hThread, &ctx);
				pParm2Addr = (LPVOID)ctx.Rdx;
				pParm3Addr = (LPVOID)ctx.R8;
				// x64의 경우, 함수의 2~3번째 인수의 주소가 Rdx, R8로 넘겨짐
#else
				ctx.Eip = (DWORD)CurrentAPI->pfAddress;
				// 0xCC가 실행된 상태임으로 증가한 EIP를 함수 주소로 맞춰줌
				SetThreadContext(hThread, &ctx);
				ReadProcessMemory(hProcess, (LPCVOID)(ctx.Esp + 0x8), &pParm2Addr, sizeof(pParm2Addr), NULL);
				ReadProcessMemory(hProcess, (LPCVOID)(ctx.Esp + 0xC), &pParm3Addr, sizeof(pParm3Addr), NULL);
				// x86의 경우, 함수의 2~3번째 인수의 주소가 스택을 통해 넘겨짐
#endif
				if (hThread) CloseHandle(hThread);

				size_t FuncNameLen = strlen(CurrentAPI->szFuncName);
				if (FuncNameLen > 0 && tolower((unsigned char)CurrentAPI->szFuncName[FuncNameLen - 1]) == 'w') {
				// 후킹할 함수가 유니코드 함수 일 경우
					wchar_t pszTestString[] = L"HOOK";
					DWORD oldProtect;
					VirtualProtectEx(hProcess, pParm2Addr, sizeof(pszTestString), PAGE_READWRITE, &oldProtect);
					WriteProcessMemory(hProcess, (LPVOID)pParm2Addr, pszTestString, sizeof(pszTestString), NULL);
					VirtualProtectEx(hProcess, pParm2Addr, sizeof(pszTestString), oldProtect, &oldProtect);
					// 2번 째 인수 값을 변조
					VirtualProtectEx(hProcess, pParm3Addr, sizeof(pszTestString), PAGE_READWRITE, &oldProtect);
					WriteProcessMemory(hProcess, (LPVOID)pParm3Addr, pszTestString, sizeof(pszTestString), NULL);
					VirtualProtectEx(hProcess, pParm3Addr, sizeof(pszTestString), oldProtect, &oldProtect);
					// 3번 째 인수 값을 변조
				} else {
				// 후킹할 함수가 ANSI 함수 일 경우
					char pszTestString[] = "HOOK";
					DWORD oldProtect;
					VirtualProtectEx(hProcess, pParm2Addr, sizeof(pszTestString), PAGE_READWRITE, &oldProtect);
					WriteProcessMemory(hProcess, (LPVOID)pParm2Addr, pszTestString, sizeof(pszTestString), NULL);
					VirtualProtectEx(hProcess, pParm2Addr, sizeof(pszTestString), oldProtect, &oldProtect);
					// 2번 째 인수 값을 변조
					VirtualProtectEx(hProcess, pParm3Addr, sizeof(pszTestString), PAGE_READWRITE, &oldProtect);
					WriteProcessMemory(hProcess, (LPVOID)pParm3Addr, pszTestString, sizeof(pszTestString), NULL);
					VirtualProtectEx(hProcess, pParm3Addr, sizeof(pszTestString), oldProtect, &oldProtect);
					// 3번 째 인수 값을 변조
				}
				return;
			}
			break;
		}
	
		case EXCEPTION_SINGLE_STEP: {
		// Single Step(Trap Flag) 예외인 경우
			printf("[I][%lu][%lu] %s : EXCEPTION_SINGLE_STEP (0x%p)\n",
				dwPID, dwTID, __func__, (LPVOID)per->ExceptionAddress);

			HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, pde->dwThreadId);
			// 스레드 핸들을 구함
			CONTEXT ctx;
			ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
			GetThreadContext(hThread, &ctx);
			ctx.EFlags &= ~0x100;
			// TrapFlag를 제거함
			SetThreadContext(hThread, &ctx);
			if (hThread) CloseHandle(hThread);
			if (gTrapApiStruct) {
				if (SetupBP(hProcess, gTrapApiStruct)) {
					printf("[I][%lu][%lu] %s : %s %s (0x%p) -> INT3 Resetup Success\n",
						dwPID, dwTID, __func__,
						gTrapApiStruct->szModuleName, gTrapApiStruct->szFuncName, gTrapApiStruct->pfAddress);
				} else {
					printf("[F][%lu][%lu] %s : %s %s (0x%p) -> INT3 Resetup Fail..\n",
						dwPID, dwTID, __func__,
						gTrapApiStruct->szModuleName, gTrapApiStruct->szFuncName, gTrapApiStruct->pfAddress);
				}
				gTrapApiStruct = NULL;
			} // BP를 재설치함.
			break;
		}
	}
	return;
}

unsigned __stdcall DebuggingThread(void* pArg){
	DWORD dwPID = GetCurrentProcessId();
	DWORD dwTID = GetCurrentThreadId();
	printf("[I][%lu][%lu] %s : Thread Loaded\n",
		dwPID, dwTID, __func__);

	DWORD dwDebugeePID = *(PDWORD)pArg;
	if (!DebugActiveProcess(dwDebugeePID)) {
		printf("[F][%lu][%lu] %s : DebugActiveProcess(%d) failed (%d)\n",
			dwPID, dwTID, __func__, dwDebugeePID, GetLastError());
		return 1;
	} // 타겟 프로세스를 디버기로 만듬

	DEBUG_EVENT de;
	HANDLE hProcess = NULL;

	while (WaitForDebugEvent(&de, INFINITE)) {
	// 디버기로부터 event 가 발생할 때까지 기다림

		switch (de.dwDebugEventCode){
			case CREATE_PROCESS_DEBUG_EVENT: {
				if (!hProcess) hProcess = ((CREATE_PROCESS_DEBUG_INFO)de.u.CreateProcessInfo).hProcess;
				printf("[I][%lu][%lu] %s : CREATE_PROCESS_DEBUG_EVENT\n",
					dwPID, dwTID, __func__);
				DebugEventSetBP(hProcess, &de, dwPID, dwTID, FALSE);
				break;
			} // 디버기의 프로세스 생성 혹은 attach 이벤트 (정적 링크 후킹)

			case LOAD_DLL_DEBUG_EVENT: {
				DebugEventSetBP(hProcess, &de, dwPID, dwTID, TRUE);
				break;
			} // 디버기의 DLL 로드 이벤트 (동적 링크 후킹)

			case EXCEPTION_DEBUG_EVENT: {
				DebugEventException(hProcess, &de, dwPID, dwTID);
				break;
			} // 디버기의 예외 이벤트 (후킹 처리)

			case EXIT_PROCESS_DEBUG_EVENT: {
				printf("[I][%lu][%lu] %s : EXIT_PROCESS_DEBUG_EVENT\n",
					dwPID, dwTID, __func__);
				return 0;
			} // 디버기의 종료 이벤트
		}

		ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
		// 디버기 스레드를 진행 시킴
	}
	return 0;
}


int main(int argc, char* argv[]) {
	DWORD dwPID = GetCurrentProcessId();
	DWORD dwTID = GetCurrentThreadId();
	DWORD dwDebugeePID;

#ifdef _WIN64
	printf("[Debugging Attach Hooking x64]\n\n");
#else
	printf("[Debugging Attach Hooking x86]\n\n");
#endif


	printf("Input Debuggee PID : ");
	scanf_s("%lu", &dwDebugeePID);
	printf("\n");

	unsigned dwSubTid;
	uintptr_t hThread = _beginthreadex(
		NULL,					// 보안 속성
		0,						// 스택 크기 (0 = 기본)
		DebuggingThread,				// 스레드 함수
		&dwDebugeePID,					// 인자
		0,						// 생성 옵션
		&dwSubTid				// 스레드 ID
	);

	if (!hThread) {
		printf("[F][%lu][%lu] %s : _beginthreadex failed.\n",
			dwPID, dwTID, __func__);
		return 1;
	}

	printf("[I][%lu][%lu] %s : Create SubThread(Tid:%lu)\n",
		dwPID, dwTID, __func__, dwSubTid);

	WaitForSingleObject((HANDLE)hThread, INFINITE);
	CloseHandle((HANDLE)hThread);

	system("pause");
	return 0;
}
