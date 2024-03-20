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
#define ATF_NO_INI_CONFIG                       0x0000000b

//
// Driver ERRORS
//
#define ATF_PARSE_INI_CONFIG                    0x10000001


// EOF