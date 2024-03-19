#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#include <ntddk.h>
#include <ntstrsafe.h>
#include <stdarg.h>

#define MAX_VSPRINTF_BUF_SIZE 512

static __inline void AtfDebugf(const char *format, ...)
{
    char buf[MAX_VSPRINTF_BUF_SIZE] = { 0 };
    NTSTATUS status;
    size_t len;
    va_list list;
    va_start(list, format);
    status = RtlStringCbVPrintfA(buf, sizeof(buf), format, list);
    if (status == STATUS_SUCCESS) {
        len = strlen(buf);
    }
    else
    {
        len = 2;
        buf[0] = 'O';
        buf[1] = '\n';
    }
    if (len)
    {
        DbgPrint("[atftrace] %s", buf);
    }
    va_end(list);
}

#define ATF_DEBUGF(format, ...) AtfDebugf(__FUNCTION__, format, ##__VA_ARGS__)

static __inline void AtfErrorf(const char *format, ...)
{
    char buf[MAX_VSPRINTF_BUF_SIZE] = { 0 };
    NTSTATUS status;
    size_t len;
    va_list list;
    va_start(list, format);
    status = RtlStringCbVPrintfA(buf, sizeof(buf), format, list);
    if (status == STATUS_SUCCESS) {
        len = strlen(buf);
    }
    else
    {
        len = 2;
        buf[0] = 'O';
        buf[1] = '\n';
    }
    if (len)
    {
        DbgPrint("[atftrace] ERROR %s", buf);
    }
    va_end(list);
}

#define ATF_ERRORF(format, ...) AtfErrorf(__FUNCTION__, format, ##__VA_ARGS__)



#define ATF_DEBUG(function, x) KdPrint(("[atftrace] " __FUNCTION__ " - " #function " dbg: %s", x))

#define ATF_ERROR(function, status) KdPrint(("[atftrace] " __FUNCTION__ " - " #function " failed (status: 0x%x)", status))

#define ATF_ASSERT(x) if (!x) { return STATUS_INVALID_PARAMETER; }
#define ATF_ASSERT_NORETURN(x) if (!x) { return; }
