#pragma once

#ifndef __ATF_COMMON__
#define __ATF_COMMON__

#include "ntstatus.h"

#define ATF_DEVICE_NAME                     L"\\Device\\atf_filter"
#define ATF_DOS_DEVICE_NAME                 L"\\DosDevices\\atf_filter"

#define SDDL_STRING                         L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"

//
// DriverController
//
#define DRIVER_CTL_NAME                     "DriverController"
#define DRIVER_SERVICE_NAME                 "ActiveTransportFilterDriver"
#define DRIVER_SERVICE_DISPLAY_NAME         "Active Transport Filter Driver"
#define DRIVER_BIN_PATH                     "ActiveTransportFilter.sys"
#define CONTROL_SERVICE_NAME                "ATFCtl"
#define CONTROL_SERVICE_DISPLAY_NAME        "Active Transport Filter Control Service"

#endif //__ATF_COMMON__