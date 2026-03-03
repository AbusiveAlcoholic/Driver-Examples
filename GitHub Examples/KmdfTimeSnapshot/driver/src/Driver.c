#include "Driver.h"

DRIVER_INITIALIZE DriverEntry;

static VOID CreateQueue(WDFDEVICE device)
{
    WDF_IO_QUEUE_CONFIG cfg;
    WDFQUEUE q;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&cfg, WdfIoQueueDispatchSequential);
    cfg.EvtIoDeviceControl = TimeEvtIoDeviceControl;
    (VOID)WdfIoQueueCreate(device, &cfg, WDF_NO_OBJECT_ATTRIBUTES, &q);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG cfg;
    WDF_DRIVER_CONFIG_INIT(&cfg, TimeEvtDeviceAdd);
    return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &cfg, WDF_NO_HANDLE);
}

NTSTATUS TimeEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);

    UNICODE_STRING ntName;
    UNICODE_STRING dosName;
    WDFDEVICE device;

    RtlInitUnicodeString(&ntName, TIME_NT_DEVICE_NAME);
    NTSTATUS status = WdfDeviceInitAssignName(DeviceInit, &ntName);
    if (!NT_SUCCESS(status)) return status;

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status)) return status;

    RtlInitUnicodeString(&dosName, TIME_DOS_DEVICE_NAME);
    status = WdfDeviceCreateSymbolicLink(device, &dosName);
    if (!NT_SUCCESS(status)) return status;

    CreateQueue(device);
    return STATUS_SUCCESS;
}

static VOID FillSnapshot(PTIME_SNAPSHOT s)
{
    LARGE_INTEGER st;
    KeQuerySystemTimePrecise(&st);
    s->SystemTime100ns = st.QuadPart;

    ULONGLONG it = KeQueryInterruptTimePrecise();
    s->InterruptTime100ns = it;

    s->BuildNumber = (ULONG)NtBuildNumber & 0xFFFF;
}

VOID TimeEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(InputBufferLength);

    NTSTATUS status = STATUS_SUCCESS;
    size_t info = 0;

    if (IoControlCode == IOCTL_TIME_PING)
    {
        status = STATUS_SUCCESS;
    }
    else if (IoControlCode == IOCTL_TIME_GET_VERSION)
    {
        PTIME_VERSION v = NULL;
        size_t outSize = 0;
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(TIME_VERSION), (PVOID*)&v, &outSize);
        if (NT_SUCCESS(status) && OutputBufferLength >= sizeof(TIME_VERSION))
        {
            v->Major = 1; v->Minor = 0; v->Patch = 0;
            info = sizeof(TIME_VERSION);
        }
        else if (NT_SUCCESS(status)) status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (IoControlCode == IOCTL_TIME_GET_SNAPSHOT)
    {
        PTIME_SNAPSHOT s = NULL;
        size_t outSize = 0;
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(TIME_SNAPSHOT), (PVOID*)&s, &outSize);
        if (NT_SUCCESS(status) && OutputBufferLength >= sizeof(TIME_SNAPSHOT))
        {
            FillSnapshot(s);
            info = sizeof(TIME_SNAPSHOT);
        }
        else if (NT_SUCCESS(status)) status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    WdfRequestCompleteWithInformation(Request, status, info);
}
