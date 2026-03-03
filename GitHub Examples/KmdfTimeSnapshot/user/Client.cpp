#include <windows.h>
#include <iostream>
#include <cstdint>

#define TIME_DEVICE_TYPE 0x8003
#define IOCTL_TIME_PING           CTL_CODE(TIME_DEVICE_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TIME_GET_VERSION    CTL_CODE(TIME_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TIME_GET_SNAPSHOT   CTL_CODE(TIME_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct TIME_VERSION { uint32_t Major, Minor, Patch; };
struct TIME_SNAPSHOT
{
    int64_t SystemTime100ns;
    uint64_t InterruptTime100ns;
    uint32_t BuildNumber;
};

int main()
{
    HANDLE dev = CreateFileW(LR"(\\.\KmdfTime)", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dev == INVALID_HANDLE_VALUE) { std::cerr << "Open failed: " << GetLastError() << "\n"; return 1; }

    DWORD ret = 0;
    TIME_SNAPSHOT s{};
    if (DeviceIoControl(dev, IOCTL_TIME_GET_SNAPSHOT, nullptr, 0, &s, sizeof(s), &ret, nullptr))
    {
        std::cout << "SystemTime100ns: " << s.SystemTime100ns << "\n";
        std::cout << "InterruptTime100ns: " << s.InterruptTime100ns << "\n";
        std::cout << "NtBuildNumber: " << s.BuildNumber << "\n";
    }
    else
    {
        std::cerr << "GET_SNAPSHOT failed: " << GetLastError() << "\n";
    }

    CloseHandle(dev);
    return 0;
}
