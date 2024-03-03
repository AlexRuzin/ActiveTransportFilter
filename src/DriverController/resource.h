#pragma once

//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by DriverController.rc

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        101
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

#if defined(_DEBUG) && defined(_M_X64)
#pragma message("MODE: DEBUG (x64)")
#define DRIVER_PATH                     "..\\..\\build\\x64\\Debug\\_driver\\ActiveTransportFilter.sys"
#elif !defined(_DEBUG) && defined(_M_X64) //!_DEBUG && _M_X64
#define DRIVER_PATH                     "..\\..\\build\\x64\\Release\\_driver\\ActiveTransportFilter.sys"
#else // Build not supported (x86)
#pragma message("MODE not supported, build will contain errors")
#pragma error "Cancelling Build"
#endif 

//
// Driver Binary (sys)
//
#define ID_DRIVER_BIN_SYS               201

//
// Service Binary
//
#define ID_CONFIG_SERVICE_BIN           202

//
// Interface/Console Binary
//
#define ID_USER_INTERFACE_CONSOLE_BIN   203
