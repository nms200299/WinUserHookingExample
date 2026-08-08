#include <Windows.h>
#include <stdio.h>

HANDLE InitTestFile() {
    HANDLE hFile = CreateFileA(
        "test.txt",
        GENERIC_ALL,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    ); // 파일 핸들 생성

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("CreateFileA failed: %lu\n", GetLastError());
        return NULL;
    }
    printf("\tCreate File Handle = %p\n", hFile);

    const char* data = "[SrcProgram] Handle creation completed!\n";
    DWORD written = 0;
    BOOL result = WriteFile(
        hFile,
        data,
        (DWORD)strlen(data),
        &written,
        NULL
    ); // 파일 쓰기

    if (result) printf("\tWriteFile Success!\n");
    return hFile;
}


HANDLE DupHandleTest(DWORD dwPidDst, HANDLE hHandleSrc, HANDLE hHandleDst) {
    HANDLE hProcessSrc = GetCurrentProcess();
    HANDLE hProcessDst = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPidDst);
    // 대상 프로그램의 프로세스 핸들을 구함

    if (DuplicateHandle(
        hProcessSrc,
        hHandleSrc,
        hProcessDst,
        &hHandleDst,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS
    )) { // 핸들 복제
        printf("\tDuplicated Handle Success (%p)\n", hHandleDst);
        return hHandleDst;
    }
    printf("\tDuplicated Handle Fail\n");
    return NULL;
}


HANDLE InitIpcClient() {
    HANDLE hPipe = CreateFileA(
        "\\\\.\\pipe\\MyPipe",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    ); // Named Pipe 접속
    if (hPipe == INVALID_HANDLE_VALUE){
        printf("CreateFileA failed: %lu\n", GetLastError());
        return NULL;
    }
    return hPipe;
}

int main() {
    printf("[SrcProgram]\n");
    DWORD dwPidDst;
    printf("Input Dst PID : ");
    scanf_s("%lu", &dwPidDst);

    HANDLE hFileOrg = InitTestFile();
    HANDLE hFileDup = NULL;
    hFileDup = DupHandleTest(dwPidDst, hFileOrg, hFileDup); 
    // 파일 핸들 생성 및 복제

    HANDLE hPipe = InitIpcClient();
    if (!hPipe) return 1;
    // Named Pipe 연결

    DWORD dwWritten = 0;
    if (WriteFile(hPipe, &hFileDup, sizeof(hFileDup), &dwWritten, NULL)) {
        printf("\tDuplicated Handle Sending Success (%lu bytes)\n", dwWritten);
    } // IPC를 통해 핸들 값 전달

    CloseHandle(hPipe);
    CloseHandle(hFileOrg);
    system("pause");
    return 0;
}