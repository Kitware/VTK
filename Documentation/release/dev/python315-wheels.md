## Python 3.15 wheels

VTK now builds and tests Python 3.15 wheels on Linux, macOS and Windows,
and free-threaded Python 3.15t wheels on Linux and macOS x86_64.

VTK does not build or publish a free-threaded Python 3.15t wheel for
Windows. The VTK test suite currently fails with that configuration, so
the wheel is not published until the incompatibility is resolved.
