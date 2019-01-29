# loguru fork for VTK

This branch contains chanegs required to embed loguru into VTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree as well as a header to facilitate mangling of the symbols to
avoid conflicts with other copies of the library within a single process.

  * ignore whitespace issues
  * integreate with VTK's module system
  * mangle `loguru` namespace to `vtkloguru`
  * add license
