#include <Windows.h>

#include "unpacker.h"
#include "../common/common.h"
#include "../common/user_logging.h"

#include <string>

int32_t GetTemporaryFilePath(std::string &out)
{
    char currPath[MAX_PATH + 1] = { 0 };
    
    const DWORD ret = GetCurrentDirectoryA(MAX_PATH, currPath);
    if (!ret) {
        return ret;
    }

    out = currPath;
    out = out + "\\" + MAIN_INSTALL_PATH + "\\";

    return 0;
}

int32_t ExtractResourceToPath(int resourceId, const std::string &resourceName, const std::string &absPath)
{
    HRSRC hResource = FindResourceA(
        GetModuleHandleA(NULL), 
        MAKEINTRESOURCEA(resourceId),
        MAKEINTRESOURCEA(RT_RCDATA)
    );

    return 0;
}

BOOL CALLBACK EnumResNameProc(HMODULE hModule, LPCSTR lpszType, LPSTR lpszName, LONG_PTR lParam) {
    if (IS_INTRESOURCE(lpszName)) {
        printf("Resource ID: %hu\n", (USHORT)lpszName);
    } else {
        wprintf(L"Resource Name: %ls\n", lpszName);
    }
    return TRUE; // Continue enumeration
}

void enumResource(void)
{
    HMODULE hModule = GetModuleHandle(NULL);
    if (!EnumResourceNamesA(hModule, RT_RCDATA, EnumResNameProc, 0)) {
        printf("Error enumerating resources: %lu\n", GetLastError());
    }
    Sleep(INFINITE);
    return;
}

