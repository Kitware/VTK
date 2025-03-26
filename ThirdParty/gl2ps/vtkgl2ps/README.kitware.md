# gl2ps fork for VTK

This branch contains changes required to embed gl2ps into VTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Add gitattributes to skip commit checks in VTK.
  * Use VTK's zlib and png libraries.
  * Integrate the CMake build with VTK's module system.
  * Mangle all exported symbols to have a `vtkgl2ps_` prefix.
