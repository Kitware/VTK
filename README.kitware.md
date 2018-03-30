double-conversion fork for VTK
---------------------

This branch contains required changes to embed double-conversion into VTK.
This includes the following:

* add `.gitattributes`
* integrate VTK's module system (added CMakeLists.vtk.txt)
* update `double-conversion/utils.h` to include exports header and mangle namespace.
* add exports to `double-conversion/utils.h and `double-conversion/double-conversion.h`.
