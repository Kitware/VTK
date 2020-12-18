# Qt6 support

The `vtkGUISupportQt` module now supports Qt6. At configure time, VTK would first look for Qt6. If
not found, it would look for Qt5 next.

Added an *internal* CMake cache variable `VTK_QT_MAJOR_VERSION` that shows the Qt version used.
