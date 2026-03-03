#include <windows.h>
#include <iostream>
#include <cstdint>

#define NOTIFY_DEVICE_TYPE 0x8001
#define IOCTL_NOTIFY_PING            CTL_CODE(NOTIFY_DEVICE_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NOTIFY_GET_VERSION     CTL_CODE(NOTIFY_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NOTIFY_SET_EVENT       CTL_CODE(NOTIFY_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NOTIFY_CLEAR_EVENT     CTL_CODE(NOTIFY_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NOTIFY_TRIGGER         CTL_CODE(NOTIFY_DEVICE_TYPE, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct NOTIFY_VERSION { uint32_t Major, Minor, Patch; };
struct NOTIFY_SET_EVENT_INPUT { HANDLE EventHandle; };

int main()
{
    HANDLE dev = CreateFileW(LR"(\\.\KmdfNotify)", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dev == INVALID_HANDLE_VALUE) { std::cerr << "Open failed: " << GetLastError() << "\n"; return 1; }

    HANDLE ev = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!ev) { std::cerr << "CreateEvent failed: " << GetLastError() << "\n"; return 1; }

    NOTIFY_SET_EVENT_INPUT in{ ev };
    DWORD ret = 0;
    if (!DeviceIoControl(dev, IOCTL_NOTIFY_SET_EVENT, &in, sizeof(in), nullptr, 0, &ret, nullptr))
    {
        std::cerr << "SET_EVENT failed: " << GetLastError() << "\n";
        return 1;
    }

    std::cout << "Waiting for kernel signal...\n";
    DeviceIoControl(dev, IOCTL_NOTIFY_TRIGGER, nullptr, 0, nullptr, 0, &ret, nullptr);

    DWORD w = WaitForSingleObject(ev, 3000);
    std::cout << "Wait result: " << w << "\n";

    DeviceIoControl(dev, IOCTL_NOTIFY_CLEAR_EVENT, nullptr, 0, nullptr, 0, &ret, nullptr);
    CloseHandle(ev);
    CloseHandle(dev);
    return 0;
}
