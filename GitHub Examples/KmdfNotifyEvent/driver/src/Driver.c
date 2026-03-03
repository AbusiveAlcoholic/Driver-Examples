#include "Driver.h"

DRIVER_INITIALIZE DriverEntry;

static VOID CreateQueue(WDFDEVICE device)
{
    WDF_IO_QUEUE_CONFIG cfg;
    WDFQUEUE q;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&cfg, WdfIoQueueDispatchSequential);
    cfg.EvtIoDeviceControl = NotifyEvtIoDeviceControl;
    (VOID)WdfIoQueueCreate(device, &cfg, WDF_NO_OBJECT_ATTRIBUTES, &q);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG cfg;
    WDF_DRIVER_CONFIG_INIT(&cfg, NotifyEvtDeviceAdd);
    return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &cfg, WDF_NO_HANDLE);
}

NTSTATUS NotifyEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);

    UNICODE_STRING ntName;
    UNICODE_STRING dosName;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES attrs;

    RtlInitUnicodeString(&ntName, NOTIFY_NT_DEVICE_NAME);
    NTSTATUS status = WdfDeviceInitAssignName(DeviceInit, &ntName);
    if (!NT_SUCCESS(status)) return status;

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrs, DEVICE_CONTEXT);
    status = WdfDeviceCreate(&DeviceInit, &attrs, &device);
    if (!NT_SUCCESS(status)) return status;

    PDEVICE_CONTEXT ctx = DeviceGetContext(device);
    ctx->UserEvent = NULL;
    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &ctx->Lock);
    if (!NT_SUCCESS(status)) return status;

    RtlInitUnicodeString(&dosName, NOTIFY_DOS_DEVICE_NAME);
    status = WdfDeviceCreateSymbolicLink(device, &dosName);
    if (!NT_SUCCESS(status)) return status;

    CreateQueue(device);
    return STATUS_SUCCESS;
}

static NTSTATUS SetUserEvent(WDFDEVICE device, HANDLE hEvent)
{
    PDEVICE_CONTEXT ctx = DeviceGetContext(device);
    PKEVENT ev = NULL;

    NTSTATUS status = ObReferenceObjectByHandle(hEvent, EVENT_MODIFY_STATE, *ExEventObjectType, UserMode, (PVOID*)&ev, NULL);
    if (!NT_SUCCESS(status)) return status;

    WdfSpinLockAcquire(ctx->Lock);
    if (ctx->UserEvent) ObDereferenceObject(ctx->UserEvent);
    ctx->UserEvent = ev;
    WdfSpinLockRelease(ctx->Lock);

    return STATUS_SUCCESS;
}

static VOID ClearUserEvent(WDFDEVICE device)
{
    PDEVICE_CONTEXT ctx = DeviceGetContext(device);

    WdfSpinLockAcquire(ctx->Lock);
    PKEVENT old = ctx->UserEvent;
    ctx->UserEvent = NULL;
    WdfSpinLockRelease(ctx->Lock);

    if (old) ObDereferenceObject(old);
}

static VOID TriggerUserEvent(WDFDEVICE device)
{
    PDEVICE_CONTEXT ctx = DeviceGetContext(device);
    PKEVENT ev = NULL;

    WdfSpinLockAcquire(ctx->Lock);
    ev = ctx->UserEvent;
    if (ev) ObReferenceObject(ev);
    WdfSpinLockRelease(ctx->Lock);

    if (ev)
    {
        KeSetEvent(ev, IO_NO_INCREMENT, FALSE);
        ObDereferenceObject(ev);
    }
}

VOID NotifyEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode)
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    NTSTATUS status = STATUS_SUCCESS;
    size_t info = 0;

    if (IoControlCode == IOCTL_NOTIFY_PING)
    {
        status = STATUS_SUCCESS;
        info = 0;
    }
    else if (IoControlCode == IOCTL_NOTIFY_GET_VERSION)
    {
        PNOTIFY_VERSION v = NULL;
        size_t outSize = 0;
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(NOTIFY_VERSION), (PVOID*)&v, &outSize);
        if (NT_SUCCESS(status) && OutputBufferLength >= sizeof(NOTIFY_VERSION))
        {
            v->Major = 1;
            v->Minor = 0;
            v->Patch = 0;
            info = sizeof(NOTIFY_VERSION);
        }
        else if (NT_SUCCESS(status))
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else if (IoControlCode == IOCTL_NOTIFY_SET_EVENT)
    {
        PNOTIFY_SET_EVENT_INPUT in = NULL;
        size_t inSize = 0;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(NOTIFY_SET_EVENT_INPUT), (PVOID*)&in, &inSize);
        if (NT_SUCCESS(status) && InputBufferLength >= sizeof(NOTIFY_SET_EVENT_INPUT))
            status = SetUserEvent(device, in->EventHandle);
    }
    else if (IoControlCode == IOCTL_NOTIFY_CLEAR_EVENT)
    {
        ClearUserEvent(device);
        status = STATUS_SUCCESS;
    }
    else if (IoControlCode == IOCTL_NOTIFY_TRIGGER)
    {
        TriggerUserEvent(device);
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    WdfRequestCompleteWithInformation(Request, status, info);
}
