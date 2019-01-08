# freetype fork for VTK

This branch contains changes required to embed freetype into VTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Add attributes to pass commit checks within VTK.
  * Use VTK's zlib library.
  * Integrate CMake code with VTK's module system.
  * Mangle all exported symbols to have a `vtkfreetype_` prefix.
