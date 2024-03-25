#pragma once

//#include <stdint.h>

typedef unsigned int ATF_ERROR;

#define ATF_ERROR_OK                            0x00000000
#define ATF_ERROR_FAIL                          0xffffffff
#define ATF_ERROR_OPEN_FILE                     0x00000001
#define ATF_ERROR_FILE_NOT_FOUND                0x00000002
#define ATF_CREATE_SERVICE                      0x00000003
#define ATF_INIT_SCM                            0x00000004
#define ATF_FAILED_PROC_CREATE                  0x00000005
#define ATF_FAILED_HANDLE_NOT_OPENED            0x00000006
#define ATF_BAD_PARAMETERS                      0x00000007
#define ATF_DEVICEIOCONTROL                     0x00000008
#define ATF_WFP_NOT_RUNNING                     0x00000009
#define ATF_WFP_ALREADY_RUNNING                 0x0000000a
#define ATF_DEFAULT_CONFIG_TOO_LARGE            0x0000000c

//
// ATF INI parser errors                        
//
#define ATF_NO_INI_CONFIG                       0x30000001
#define ATF_BAD_INI_CONFIG                      0x30000002

//
// Driver ERRORS
//
#define ATF_PARSE_INI_CONFIG                    0x10000001
#define ATF_CORRUPT_CONFIG                      0x10000002
#define ATF_NO_MEMORY_AVAILABLE                 0x10000003
#define ATF_IOCTL_BUFFER_TOO_LARGE              0x10000004

//
// WFP signals
//
#define ATF_FILTER_SIGNAL_PASS                  0x20000000
#define ATF_FILTER_SIGNAL_BLOCK                 0x20000001
#define ATF_FILTER_SIGNAL_ALERT                 0x20000002

// EOF