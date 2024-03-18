#include <Windows.h>

#include "config_service.h"

#include "driver_comm.h"
#include "../common/user_logging.h"
#include "../common/common.h"
#include "../common/errors.h"
#include "../common/shared.h"
#include "ini_reader.h"


const std::shared_ptr<IoctlComm> &ConfigRefreshService::GetIoctlComm(void) const
{
    return ioctlComm;
}