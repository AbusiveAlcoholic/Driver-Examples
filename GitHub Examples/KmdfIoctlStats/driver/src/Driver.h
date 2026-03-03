#pragma once
#include <ntddk.h>
#include <wdf.h>
#include "Ioctl.h"

#define STATS_NT_DEVICE_NAME   L"\\Device\\KmdfStats"
#define STATS_DOS_DEVICE_NAME  L"\\DosDevices\\KmdfStats"

EVT_WDF_DRIVER_DEVICE_ADD StatsEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL StatsEvtIoDeviceControl;

typedef struct _DEVICE_CONTEXT
{
    STATS_COUNTERS C;
    WDFSPINLOCK Lock;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)
