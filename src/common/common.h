#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

//
// Central configuration for the entire project
//  Contains mostly strings and constants that are shared between service, driver, etc
//


//
// ActiveTransportFilter Driver
//
#define ATF_DEVICE_NAME                     "\\Device\\atf_filter"
#define ATF_DOS_DEVICE_NAME                 "\\DosDevices\\atf_filter"

#define SDDL_STRING                         "D:P(A;;GA;;;SY)(A;;GA;;;BA)"


//
// FWPM Engine Registration (filtering engine registered at drive level)
//
#define DRIVER_FWPM_SERVICENAME             "atf_filter_service"
#define DRIVER_FWPM_DISPLAYNAME             L"atf_filter_provider"
#define DRIVER_FWPM_DESC                    L"ATF Provider Object"



//
// DriverController
//
#define DRIVER_CTL_NAME                     "DriverController"
#define DRIVER_SERVICE_NAME                 "ActiveTransportFilterDriver"
#define DRIVER_SERVICE_DISPLAY_NAME         "Active Transport Filter Driver"
#define DRIVER_BIN_PATH                     "ActiveTransportFilter.sys"


//
// DeviceConfigService
//  Service that sends configurations to the driver via IOCTL
//
#define CONTROL_SERVICE_NAME                "ATFCtl"
#define CONTROL_SERVICE_DISPLAY_NAME        "Active Transport Filter Control Service"


//
// Temporary directory and install path, this is usually "temp" at the same location as the 
//  DriverControler executable
//
#define MAIN_INSTALL_PATH                   "atf_bin_directory"   


//
// Dynamic and filter configuration ini file
//
#define GLOBAL_IP_FILTER_INI                MAIN_INSTALL_PATH "\\filter_config.ini"
#define GLOBAL_IP_FILTER_INI_DEBUG          "..\\..\\config\\filter_config.ini"


//
// Image names
//
#define FILENAME_ATF_DRIVER                 "ActiveTransportFilter.sys"
#define FILENAME_ATF_DRIVER_CAT             "ActiveTransportFilter.cat"
#define FILENAME_DEVICE_CONFIG_SERVICE      "DeviceConfigService.exe"
#define FILENAME_INTERFACE_CONSOLE          "InterfaceConsole.exe"
