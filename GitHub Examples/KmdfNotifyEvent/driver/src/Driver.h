#pragma once
#include <ntddk.h>
#include <wdf.h>
#include "Ioctl.h"

#define NOTIFY_NT_DEVICE_NAME   L"\\Device\\KmdfNotify"
#define NOTIFY_DOS_DEVICE_NAME  L"\\DosDevices\\KmdfNotify"

EVT_WDF_DRIVER_DEVICE_ADD NotifyEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL NotifyEvtIoDeviceControl;

typedef struct _DEVICE_CONTEXT
{
    PKEVENT UserEvent;
    WDFSPINLOCK Lock;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)
