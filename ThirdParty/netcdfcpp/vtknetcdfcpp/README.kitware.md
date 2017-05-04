# NetCDF-3 C++ for for VTK

This directory contains the NetCDF-3 C++ library with changes to support
embedding it VTK. This includes changes made primarily to the build system to
allow it to be embedded into another source tree as well as a header to
facilitate mangling of the symbols to avoid conflicts with other copies of the
library within a single process.

  * Add attributes to pass commit checks within VTK.
  * Removal of `config.h` since none of its symbols are used.
  * Add a CMake build system to the project.
  * Export symbols for Windows support.
  * Mangle all exported symbols to have a `vtknetcdfcxx_` prefix.
