# fast_float fork for VTK

This branch contains changes required to embed fast_float into VTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

* Add `.gitattributes` to avoid failures in VTK's content checks.
* Integrate the CMake build with VTK's module system.
* Mangle all exported symbols to live in a `vtkfast_float` namespace.
