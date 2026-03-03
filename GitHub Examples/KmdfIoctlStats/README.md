# KmdfIoctlStats

- KMDF device that tracks per-IOCTL counters
- Thread-safe state using a spinlock
- Clean example of structured output buffers

Device: \\.\KmdfStats

IOCTLs: PING, GET_VERSION, GET_COUNTERS, RESET_COUNTERS

Credits: Frank Gallagher
License: MIT (see LICENSE)
