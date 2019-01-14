# JsonCpp fork for VTK

This branch contains changes required to embed JsonCpp into VTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree as well as a header to facilitate mangling of the symbols
to avoid conflicts with other copies of the library within a single process.

  * Add attributes to pass commit checks within VTK.
  * Integrate with VTK's module system.
  * Mangle symbols to use the `vtkJson` namespace instead of `Json`.
