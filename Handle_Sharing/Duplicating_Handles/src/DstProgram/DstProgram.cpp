#include <Windows.h>
#include <stdio.h>

HANDLE InitIpcServer() {
    HANDLE hPipe = CreateNamedPipeA(
        "\\\\.\\pipe\\MyPipe",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        4096, 4096,
        0,
        NULL
    );
    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("\tCreateNamedPipeA failed: %lu\n", GetLastError());
        return NULL;
    }
    printf("\tCreateNamedPipeA Success\n");
    // Named Pipe 생성

    BOOL result = ConnectNamedPipe(hPipe, NULL);
    // 연결될 때까지 대기
    if (!result) {
        DWORD error = GetLastError();
        if (error != ERROR_PIPE_CONNECTED) {
            printf("ConnectNamedPipe failed: %lu\n", error);
            CloseHandle(hPipe);
            return NULL;
        } // ConnectNamedPipe 호출 전에 이미 연결한 경우
    }
    return hPipe;
}


int main() {
	printf("[DstProgram]\n");
	printf("PID : %d\n", GetCurrentProcessId());

    HANDLE hPipe = InitIpcServer();
    if (!hPipe) return 1;

    DWORD dwRead;
    HANDLE hHandleDst=NULL;

    if (ReadFile(hPipe, &hHandleDst, sizeof(hHandleDst), &dwRead, NULL)){
        // Named Pipe로 부터 전달된 핸들 값을 읽음
        printf("\tDuplicated Handle Recive Success (%lu bytes)\n", dwRead);
        if (!hHandleDst) return 1;
        printf("\tDuplicated Handle (%p)\n", hHandleDst);

        const char* data = "[DstProgram] Duplicated Handle Success!\n";
        DWORD written = 0;
        BOOL result = WriteFile(
            hHandleDst,
            data,
            (DWORD)strlen(data),
            &written,
            NULL
        ); // 해당 핸들로 파일 쓰기

        if (result) printf("\tWriteFile Success!\n");
        CloseHandle(hHandleDst);
    }
    CloseHandle(hPipe);
    system("pause");
    return 0;
}