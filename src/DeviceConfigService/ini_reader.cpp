#include <Windows.h>

#include <INIReader.h>

#include "ini_reader.h"

#include "../common/errors.h"
#include "../common/user_driver_transport.h"

ATF_ERROR FilterConfig::ParseIniFile(void)
{
    return ATF_ERROR_OK;
}

const USER_DRIVER_FILTER_TRANSPORT_DATA &FilterConfig::GetRawFilterData(void) const
{
    return rawTransportData;
}