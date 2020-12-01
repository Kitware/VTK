# cgns fork for ParaView

This branch contains changes required to embed cgns into ParaView. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Add attributes to pass commit checks within ParaView.
  * Use VTK's vtkhdf5 module.
  * Remove build status badges from the readme.
  * Integrate with the VTK module system.
  * Mangle all exported symbols to have a `vtkcgns_` prefix.
