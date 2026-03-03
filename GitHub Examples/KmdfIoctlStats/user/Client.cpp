#include <windows.h>
#include <iostream>
#include <cstdint>

#define STATS_DEVICE_TYPE 0x8002
#define IOCTL_STATS_PING            CTL_CODE(STATS_DEVICE_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STATS_GET_VERSION     CTL_CODE(STATS_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STATS_GET_COUNTERS    CTL_CODE(STATS_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STATS_RESET_COUNTERS  CTL_CODE(STATS_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct STATS_VERSION { uint32_t Major, Minor, Patch; };
struct STATS_COUNTERS
{
    uint64_t IoctlPing;
    uint64_t IoctlGetVersion;
    uint64_t IoctlGetCounters;
    uint64_t IoctlResetCounters;
    uint64_t InvalidIoctl;
};

int main()
{
    HANDLE dev = CreateFileW(LR"(\\.\KmdfStats)", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dev == INVALID_HANDLE_VALUE) { std::cerr << "Open failed: " << GetLastError() << "\n"; return 1; }

    DWORD ret = 0;
    DeviceIoControl(dev, IOCTL_STATS_PING, nullptr, 0, nullptr, 0, &ret, nullptr);

    STATS_VERSION v{};
    if (DeviceIoControl(dev, IOCTL_STATS_GET_VERSION, nullptr, 0, &v, sizeof(v), &ret, nullptr))
        std::cout << "Version: " << v.Major << "." << v.Minor << "." << v.Patch << "\n";

    STATS_COUNTERS c{};
    if (DeviceIoControl(dev, IOCTL_STATS_GET_COUNTERS, nullptr, 0, &c, sizeof(c), &ret, nullptr))
    {
        std::cout << "PING: " << c.IoctlPing << "\n";
        std::cout << "GET_VERSION: " << c.IoctlGetVersion << "\n";
        std::cout << "GET_COUNTERS: " << c.IoctlGetCounters << "\n";
        std::cout << "RESET_COUNTERS: " << c.IoctlResetCounters << "\n";
        std::cout << "INVALID: " << c.InvalidIoctl << "\n";
    }

    CloseHandle(dev);
    return 0;
}
