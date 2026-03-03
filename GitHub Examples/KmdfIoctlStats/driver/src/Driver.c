#include "Driver.h"

DRIVER_INITIALIZE DriverEntry;

static VOID CreateQueue(WDFDEVICE device)
{
    WDF_IO_QUEUE_CONFIG cfg;
    WDFQUEUE q;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&cfg, WdfIoQueueDispatchSequential);
    cfg.EvtIoDeviceControl = StatsEvtIoDeviceControl;
    (VOID)WdfIoQueueCreate(device, &cfg, WDF_NO_OBJECT_ATTRIBUTES, &q);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG cfg;
    WDF_DRIVER_CONFIG_INIT(&cfg, StatsEvtDeviceAdd);
    return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &cfg, WDF_NO_HANDLE);
}

NTSTATUS StatsEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);

    UNICODE_STRING ntName;
    UNICODE_STRING dosName;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES attrs;

    RtlInitUnicodeString(&ntName, STATS_NT_DEVICE_NAME);
    NTSTATUS status = WdfDeviceInitAssignName(DeviceInit, &ntName);
    if (!NT_SUCCESS(status)) return status;

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrs, DEVICE_CONTEXT);
    status = WdfDeviceCreate(&DeviceInit, &attrs, &device);
    if (!NT_SUCCESS(status)) return status;

    PDEVICE_CONTEXT ctx = DeviceGetContext(device);
    RtlZeroMemory(&ctx->C, sizeof(ctx->C));
    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &ctx->Lock);
    if (!NT_SUCCESS(status)) return status;

    RtlInitUnicodeString(&dosName, STATS_DOS_DEVICE_NAME);
    status = WdfDeviceCreateSymbolicLink(device, &dosName);
    if (!NT_SUCCESS(status)) return status;

    CreateQueue(device);
    return STATUS_SUCCESS;
}

static VOID Inc(PDEVICE_CONTEXT ctx, ULONGLONG* field)
{
    WdfSpinLockAcquire(ctx->Lock);
    (*field)++;
    WdfSpinLockRelease(ctx->Lock);
}

VOID StatsEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(InputBufferLength);

    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT ctx = DeviceGetContext(device);

    NTSTATUS status = STATUS_SUCCESS;
    size_t info = 0;

    if (IoControlCode == IOCTL_STATS_PING)
    {
        Inc(ctx, &ctx->C.IoctlPing);
        status = STATUS_SUCCESS;
    }
    else if (IoControlCode == IOCTL_STATS_GET_VERSION)
    {
        Inc(ctx, &ctx->C.IoctlGetVersion);
        PSTATS_VERSION v = NULL;
        size_t outSize = 0;
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(STATS_VERSION), (PVOID*)&v, &outSize);
        if (NT_SUCCESS(status) && OutputBufferLength >= sizeof(STATS_VERSION))
        {
            v->Major = 1; v->Minor = 0; v->Patch = 0;
            info = sizeof(STATS_VERSION);
        }
        else if (NT_SUCCESS(status)) status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (IoControlCode == IOCTL_STATS_GET_COUNTERS)
    {
        Inc(ctx, &ctx->C.IoctlGetCounters);
        PSTATS_COUNTERS out = NULL;
        size_t outSize = 0;
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(STATS_COUNTERS), (PVOID*)&out, &outSize);
        if (NT_SUCCESS(status) && OutputBufferLength >= sizeof(STATS_COUNTERS))
        {
            WdfSpinLockAcquire(ctx->Lock);
            *out = ctx->C;
            WdfSpinLockRelease(ctx->Lock);
            info = sizeof(STATS_COUNTERS);
        }
        else if (NT_SUCCESS(status)) status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (IoControlCode == IOCTL_STATS_RESET_COUNTERS)
    {
        Inc(ctx, &ctx->C.IoctlResetCounters);
        WdfSpinLockAcquire(ctx->Lock);
        RtlZeroMemory(&ctx->C, sizeof(ctx->C));
        WdfSpinLockRelease(ctx->Lock);
        status = STATUS_SUCCESS;
    }
    else
    {
        Inc(ctx, &ctx->C.InvalidIoctl);
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    WdfRequestCompleteWithInformation(Request, status, info);
}
