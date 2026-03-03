#pragma once
#include <ntddk.h>
#include <wdf.h>
#include "Ioctl.h"

#define TIME_NT_DEVICE_NAME   L"\\Device\\KmdfTime"
#define TIME_DOS_DEVICE_NAME  L"\\DosDevices\\KmdfTime"

EVT_WDF_DRIVER_DEVICE_ADD TimeEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL TimeEvtIoDeviceControl;
