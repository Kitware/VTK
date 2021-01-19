# Qt6 support

VTK now supports Qt6.

(Re)Introduced a CMake variable `VTK_QT_VERSION` that can be set to one of *Auto*(default), *6* or
*5*. If set to *Auto*, VTK will automatically deduce the installed Qt version. When both Qt5 and
Qt6 are locatable by CMake, VTK will prefer Qt6. This behavior can be changed by setting the
`VTK_QT_VERSION` directly to the desired version (5 or 6).
