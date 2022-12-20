## `find_package(VTK)` now verifies component requests

The `vtk-config.cmake` CMake package will no longer permit unknown components
to be listed and will report them as not found.
