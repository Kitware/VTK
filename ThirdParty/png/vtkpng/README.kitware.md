# PNG fork for VTK

This branch contains changes required to embed PNG into VTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree as well as a header to facilitate mangling of the symbols
to avoid conflicts with other copies of the library within a single process.

  * Add attributes to pass commit checks within VTK.
  * Addition of a CMake build for building within VTK.
  * Mangle all exported symbols to have a `vtk_png_` prefix.
  * Support for using VTK's import of zlib.
