#include <Windows.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
	if (argc != 2) return 1;
    printf("[Child Process]\n");

	HANDLE hFile = (HANDLE)strtoull(argv[1], NULL, 16);
    printf("\tInheritance File Handle = %p\n", hFile);

    const char* data = "[Child_Process] Handle inheritance succeeded!\n";
    DWORD written = 0;
    BOOL result = WriteFile(
        hFile,
        data,
        (DWORD)strlen(data),
        &written,
        NULL
    );
   
    if (result) printf("\tWriteFile Success!\n");

    CloseHandle(hFile);
    system("pause");
	return 0;
}


