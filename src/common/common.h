#pragma once

#ifndef __ATF_COMMON__
#define __ATF_COMMON__

#include "ntstatus.h"

#define ATF_DEVICE_NAME         L"\\Device\\atf_filter"
#define ATF_DOS_DEVICE_NAME     L"\\DosDevices\\atf_filter"

#define SDDL_STRING             L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"

#endif //__ATF_COMMON__