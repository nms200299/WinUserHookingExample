#include <Windows.h>
#include <stdio.h>

HANDLE InitTestFile() {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    // 핸들 상속 설정

    HANDLE hFile = CreateFileA(
        "test.txt",
        GENERIC_ALL,
        0,
        &sa, // lpSecurityAttributes
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    ); // 상속 가능한 파일 핸들 생성

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("CreateFileA failed: %lu\n", GetLastError());
        return NULL;
    }
    printf("\tCreate File Handle = %p\n", hFile);

    const char* data = "[Parent_Process] Handle creation completed!\n";
    DWORD written = 0;
    BOOL result = WriteFile(
        hFile,
        data,
        (DWORD)strlen(data),
        &written,
        NULL
    ); // 부모 프로세스 파일 쓰기

    if (result) printf("\tWriteFile Success!\n");

    return hFile;
}

VOID CreateChildProcess(HANDLE hFile) {
    STARTUPINFOA si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(STARTUPINFOA);

    char cmdLine[MAX_PATH] = {0,};
    sprintf_s(cmdLine, "Child_Process.exe %p", hFile);

    BOOL bResult = CreateProcessA(
        NULL,
        cmdLine,
        NULL,
        NULL,
        TRUE, // bInheritHandles
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si,
        &pi
    ); // 상속 가능한 핸들을 상속하는 프로세스 생성

    if (!bResult) return;
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}


int main() {
    printf("[Parent Process]\n");

    HANDLE hFile = InitTestFile();
    CreateChildProcess(hFile);
    CloseHandle(hFile);

    system("pause");
    return 0;
}