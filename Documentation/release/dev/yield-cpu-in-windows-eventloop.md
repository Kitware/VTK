## Reduce CPU usage in Win32 event loop

VTK no longer busy-waits in the Win32 event loop, significantly reducing CPU usage when the application is idle. The event loop now uses `WaitMessage()` to sleep when no events are pending, instead of continuously spinning the CPU.

See [discourse discussion](https://discourse.vtk.org/t/cpu-usage-is-high-since-v9-5-0/16367).
