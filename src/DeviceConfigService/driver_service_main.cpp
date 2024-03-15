#include <Windows.h>

#include "driver_service_main.h"
#include "../common/user_logging.h"
#include "../common/common.h"
#include "../common/errors.h"
#include "ini_reader.h"

//
// Returns an ini file, either it's in debug mode or it's in packed mode
//
ATF_ERROR getIniFile(std::string &outPath);

bool isFileAvail(std::string file);

int main(void)
{
    LOG_INIT(CONTROL_SERVICE_NAME, LOG_SOURCE_WINDOWS_DEBUG);
    LOG_INFO("Starting configuration service %s", CONTROL_SERVICE_NAME);

    std::string targetIniFile;
    ATF_ERROR atfError = getIniFile(targetIniFile);
    if (atfError) {
        LOG_ERROR("Failed to open ini: 0x%08x", atfError);
        return atfError;
    }
    
    LOG_INFO("Found ini config path: %s", targetIniFile);

    FilterConfig filterConfig(targetIniFile);
    atfError = filterConfig.ParseIniFile();
    if (atfError) {
        LOG_ERROR("Failed to parse INI: 0x%08x", atfError);
        return atfError;
    }


    return 0;
}

ATF_ERROR getIniFile(std::string &outPath)
{
    if (isFileAvail(GLOBAL_IP_FILTER_INI)) {
        outPath = GLOBAL_IP_FILTER_INI;
    } else if (isFileAvail(GLOBAL_IP_FILTER_INI_DEBUG)) {
        outPath = GLOBAL_IP_FILTER_INI_DEBUG;
    } else {
        return ATF_ERROR_FILE_NOT_FOUND;
    }

    return ATF_ERROR_OK;
}

bool isFileAvail(std::string file) {
    DWORD dwAttrib = GetFileAttributes(file.c_str());

    return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}