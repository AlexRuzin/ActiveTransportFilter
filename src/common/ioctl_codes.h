#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

//
// This file contains IOCTLs that are used by user-mode to transport data from usermode to the driver
//  See common.h for additional logical device strings required for obtaining a driver handle
//

//
// Usermode to driver periodic keepalive 
//
#define IOCTL_ATF_SEND_KEEPALIVE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

//
// WFP Configuration Call
//  Will provide the USER_DRIVER_FILTER_TRANSPORT_DATA in specific (see user_driver_transport.h for definition)
//
#define IOCTL_ATF_SEND_WFP_CONFIG \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
