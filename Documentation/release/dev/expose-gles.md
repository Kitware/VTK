## Expose VTK_OPENGL_USE_GLES

The VTK CMake config file now properly expose `VTK_OPENGL_USE_GLES` so it's possible
to retrieve its value when doing `find_package(VTK)`.
