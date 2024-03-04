#pragma once

#ifndef __ATF_TRACE__
#define __ATF_TRACE__

#include <ntddk.h>
#include <wdf.h>

#define ATF_DEBUG(function, x) KdPrint(("[atftrace] " __FUNCTION__ " - " #function " dbg: %s", x))

#define ATF_ERROR(function, status) KdPrint(("[atftrace] " __FUNCTION__ " - " #function " failed (status: 0x%x)", status))

#define ATF_ASSERT(x) if (!x) { return STATUS_INVALID_PARAMETER; }
#define ATF_ASSERT_NORETURN(x) if (!x) { return; }

#endif //__ATF_TRACE__