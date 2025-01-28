# cgns fork for VTK

This branch contains changes required to embed cgns into VTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree as well as a header to facilitate mangling of the symbols
to avoid conflicts with other copies of the library within a single process.

  * Add attributes to pass commit checks within VTK.
  * Use VTK's vtkhdf5 module.
  * Remove build status badges from the readme.
  * Mangle all exported symbols to have a `vtkcgns_` prefix.
  * Integrate with the VTK module system.
