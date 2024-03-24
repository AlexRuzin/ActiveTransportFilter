#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

//
// Central configuration for the entire project
//  Contains mostly strings and constants that are shared between service, driver, etc
//


//
// ActiveTransportFilter Driver name and symbolic links
//
#define ATF_DRIVER_NAME                     "atf_filter"
#define ATF_DEVICE_NAME                     "\\Device\\" ATF_DRIVER_NAME
#define ATF_DOS_DEVICE_NAME                 "\\DosDevices\\" ATF_DRIVER_NAME

#define SDDL_STRING                         "D:P(A;;GA;;;SY)(A;;GA;;;BA)"


//
// FWPM Engine Registration (filtering engine registered at drive level)
//
#define DRIVER_FWPM_SERVICENAME             "atf_filter_service"
#define DRIVER_FWPM_DISPLAYNAME             "atf_filter_provider"
#define DRIVER_FWPM_DESC                    "ATF Provider Object"



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
#define CONTROL_SERVICE_SLEEP_MS            3000 // Sleep time until the control service attempts to communicate with device
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
#define FILENAME_CONFIG                     "filter_config.ini"

//
// Dynamic and filter configuration ini file
//
#define GLOBAL_IP_FILTER_INI                FILENAME_CONFIG
#define GLOBAL_IP_FILTER_INI_DEBUG          "..\\..\\config\\" FILENAME_CONFIG
#define GLOBAL_IP_FILTER_INI_RUNTIME        ".\\" MAIN_INSTALL_PATH "\\" FILENAME_CONFIG

//
// Maximum number of blacklist addresses
//
#define GLOBAL_MAX_IPV4_ADDRESSES           65536 // 2^16
#define GLOBAL_MAX_IPV6_ADDRESSES           16384 // 2^16

//
// Max number of WFP layer descriptors. In reality this driver will not need to hook more than
//  several layers, but adding 256 "just in case"
//
#define MAX_CALLOUT_LAYER_DATA              256 // We will never need more than 256 layer descriptors