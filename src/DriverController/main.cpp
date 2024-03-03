#include <Windows.h>

#include <stdint.h>

#include "service.h"
#include "../common/user_logging.h"
#include "../common/common.h"

int32_t main(int32_t argc, char argv[])
{
    LOG_INIT(DRIVER_CTL_NAME, LOG_SOURCE_WINDOWS_DEBUG);

    LOG_WARNING("Test %s", "asdfasdf");
    LOG_WARNING("nope");
    LOG_DEFAULT("TEST");
    LOG_DEFAULT(std::string("test"));
    std::string test3 = "asdfasdf";
    LOG_DEFAULT(test3 + DRIVER_CTL_NAME + "asdfasdf");

    int val = 0x02342;
    LOG_ERROR("Severe error: 0x%08x", val);

    LOG_INFO("Test %s", "asdfasdf");

    Logger::GetInstance() << "test";

    test();

    return 0;
}