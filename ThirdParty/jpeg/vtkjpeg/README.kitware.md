# jpeg-turbo fork for VTK

This branch contains changes required to embed jpeg-turbo into VTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Add a guard for duplicate `jmorecfg.h` inclusion.
  * Integrate the CMake build with VTK's module system.
  * Mangle all exported symbols to have a `vtkjpeg_` prefix.
