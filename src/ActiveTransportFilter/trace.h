#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#include <ntddk.h>
#include <ntstrsafe.h>
#include <stdarg.h>

#define MAX_VSPRINTF_BUF_SIZE 512



//#define ATF_DEBUGAF(format, ...) KdPrint(__FUNCTION__, format, ##__VA_ARGS__)
#define ATF_DEBUGA(format, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, format, ##__VA_ARGS__)



#define ATF_DEBUG(function, x) KdPrint(("[atftrace] " __FUNCTION__ " - " #function " dbg: %s\n", x))
#define ATF_DEBUGD(function, x) KdPrint(("[atftrace] " __FUNCTION__ " - " #function " dbg: %d\n", x))

#define ATF_ERROR(function, status) KdPrint(("[atftrace] " __FUNCTION__ " - " #function " failed (status: 0x%x)\n", status))

#define ATF_ASSERT(x) if (!x) { return STATUS_INVALID_PARAMETER; }
#define ATF_ASSERT_NORETURN(x) if (!x) { return; }
