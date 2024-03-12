#pragma once

#ifndef __ATF_COMMON__
#define __ATF_COMMON__

//
// ActiveTransportFilter Driver
//
#define ATF_DEVICE_NAME                     L"\\Device\\atf_filter"
#define ATF_DOS_DEVICE_NAME                 L"\\DosDevices\\atf_filter"

#define SDDL_STRING                         L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"

//
// FWPM Engine Registration (filtering engine registered at drive level)
//
#define DRIVER_FWPM_SERVICENAME             L"atf_filter_service"
#define DRIVER_FWPM_DISPLAYNAME             L"atf_filter_provider"
#define DRIVER_FWPM_DESC                    L"ATF Provider Object"



//
// DriverController
//
#define DRIVER_CTL_NAME                     "DriverController"
#define DRIVER_SERVICE_NAME                 "ActiveTransportFilterDriver"
#define DRIVER_SERVICE_DISPLAY_NAME         "Active Transport Filter Driver"
#define DRIVER_BIN_PATH                     "ActiveTransportFilter.sys"
#define CONTROL_SERVICE_NAME                "ATFCtl"
#define CONTROL_SERVICE_DISPLAY_NAME        "Active Transport Filter Control Service"

//
// Temporary directory and install path, this is usually "temp" at the same location as the 
//  DriverControler executable
//
#define MAIN_INSTALL_PATH                   "atf_bin_directory"   

//
// Image names
//
#define FILENAME_ATF_DRIVER                 "ActiveTransportFilter.sys"
#define FILENAME_ATF_DRIVER_CAT             "ActiveTransportFilter.cat"
#define FILENAME_DEVICE_CONFIG_SERVICE      "DeviceConfigService.exe"
#define FILENAME_INTERFACE_CONSOLE          "InterfaceConsole.exe"

#endif //__ATF_COMMON__