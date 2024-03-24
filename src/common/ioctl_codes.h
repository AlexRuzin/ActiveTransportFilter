#pragma once

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
// Start the WFP service command
//  Signals the driver, with the current configuration, to initialize WFP callbacks and begin filtering
//
#define IOCTL_ATF_WFP_SERVICE_START \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

//
// Stop the WFP service
//  Stop the WFP service, but preserve the configuration
//
#define IOCTL_ATF_WFP_SERVICE_STOP \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

//
// Flush the WFP configuration
//  Flushes the config. Note: the WFP service can be running while updating or flushing the config
//
#define IOCTL_ATF_FLUSH_CONFIG \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

//
// WFP Configuration Call
//  Will provide the USER_DRIVER_FILTER_TRANSPORT_DATA in specific (see user_driver_transport.h for definition)
//  Note: The WFP service can be running while updating or modifying the config
//
#define IOCTL_ATF_SEND_WFP_CONFIG \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

//
// Add blacklist IP call
//  This call will send an array of ipv4 addresses to be blacklisted by the filter. The IPs are supplied
//  by a blacklist from a DNSBL or IP blocklist, and sent in big-endian format to the filter device.
// 
// Note: this call can be invoked multiple times, if the blacklist is too large, each subsequent call will
//  append IPs into the driver's memory until complete. From there, to start the engine, call on
//  the start WFP call (IOCTL_ATF_WFP_SERVICE_START)
// 
// Note: Before this call can succeed, a default config must be set, so IOCTL_ATF_SEND_WFP_CONFIG
//  needs to be called.
// 
// If there exists a running filter.c version, then the service must be stopped, config must be flushed,
//  (IOCTL_ATF_FLUSH_CONFIG), new config added, new blacklist IPs added, and service must be started
//  (IOCTL_ATF_WFP_SERVICE_START)
//
#define IOCTL_ATF_APPEND_IPV4_BLACKLIST \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

//EOF