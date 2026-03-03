# KmdfNotifyEvent

- KMDF device with a user-provided event handle
- Driver can signal the event on demand (portfolio-safe notification pattern)
- Shows ObReferenceObjectByHandle + synchronization

Device: \\.\KmdfNotify

IOCTLs: PING, GET_VERSION, SET_EVENT, CLEAR_EVENT, TRIGGER

Credits: Frank Gallagher
License: MIT (see LICENSE)
