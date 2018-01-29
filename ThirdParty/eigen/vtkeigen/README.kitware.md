# Eigen fork for VTK

This branch contains changes required to embed Eigen into VTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Add `.gitattributes` to avoid failures in VTK's content checks.
  * Remove executable permissions from include files.
  * Integrate the CMake build with VTK's module system.
  * Mangle all exported symbols to live in a `vtkeigen` namespace.
