# zlib fork for VTK

This branch contains changes required to embed zlib into VTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree as well as a header to facilitate mangling of the symbols
to avoid conflicts with other copies of the library within a single process.

  * Modifications of some includes to work inside of VTK;
  * Some whitespace fixups;
  * Blocking out flags which VTK does not care about in CMake primarily:
    - following VTK's build options rather than exposing more options to
      users; and
    - building and installing via VTK's mechanisms.
  * Mangle all exported symbols to have a `vtkzlib_` prefix.
