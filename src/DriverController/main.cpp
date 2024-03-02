#include <Windows.h>

#include <stdint.h>

#include "service.h"
#include "../common/user_logging.h"
#include "../common/common.h"

int32_t main(int32_t argc, char argv[])
{
    LOG_INIT(DRIVER_CTL_NAME, Logger::_logging_type::_logging_type_windows_debug);

    std::string test3 = "asdfasdf";
    LOG_DEBUG(test3 + DRIVER_CTL_NAME + "asdfasdf");

    LOG_PRINTF("Test %s", "asdfasdf");

    Logger::GetInstance() << "test";

    test();

    return 0;
}