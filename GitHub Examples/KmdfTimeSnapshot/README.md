# KmdfTimeSnapshot

- KMDF service that returns timing + OS build snapshot to user-mode
- Uses KeQuerySystemTimePrecise and KeQueryInterruptTimePrecise
- Nice demo of returning structured data safely

Device: \\.\KmdfTime

IOCTLs: PING, GET_VERSION, GET_SNAPSHOT

Credits: Frank Gallagher
License: MIT (see LICENSE)
