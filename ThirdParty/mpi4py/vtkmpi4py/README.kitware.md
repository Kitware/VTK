# mpi4py fork for VTK

This branch contains changes required to embed mpi4py into VTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Remove profiling support; VTK doesn't need it.
  * Ignore errors in VTK's commit checks.
  * Integrate the CMake build with VTK's module system.
