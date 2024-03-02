#include <Windows.h>

#include <stdint.h>

#include "service.h"
#include "../common/user_logging.h"
#include "../common/common.h"

int32_t main(int32_t argc, char argv[])
{
    LOG_INIT(DRIVER_CTL_NAME, Logger::_logging_type::_logging_type_windows_debug);
    LOG_DEBUG("Test");

    test();

    return 0;
}