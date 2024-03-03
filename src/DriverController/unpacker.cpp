#include <Windows.h>

#include "unpacker.h"
#include "../common/common.h"
#include "../common/user_logging.h"

#include <string>
#include <fstream>

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

int32_t ExtractResourceToPath(int resourceId, const std::string &absPath)
{
    HMODULE modHandle = GetModuleHandleA(NULL);

    HRSRC resHandle = FindResourceA(
        modHandle,
        MAKEINTRESOURCEA(resourceId),
        RT_RCDATA

    );
    if (!resHandle) {
        LOG_ERROR("Error FindResourceA: 0x%08x", GetLastError());
        return -1;
    }

    HGLOBAL memHandle = LoadResource(modHandle, resHandle);
    if (!memHandle) {
        LOG_ERROR("Error LoadResource: 0x%08x", GetLastError());
        return -1;
    }

    const DWORD resourceSize = SizeofResource(modHandle, resHandle);
    if (resourceSize == 0) {
        LOG_ERROR("Error: resource %s size is %d", absPath, resourceSize);
        return -1;
    }

    void *data = LockResource(memHandle);
    if (!data) {
        LOG_ERROR("Error LockResource returned NULL for " + absPath);
        return -1;
    }

    std::ofstream outFile(absPath, std::ios::binary);
    if (!outFile.is_open()) {
        LOG_ERROR("Failed to open file: " + absPath);
        return -1;
    }

    outFile.write(reinterpret_cast<const char *>(data), resourceSize);
    outFile.close();

    return 0;
}

