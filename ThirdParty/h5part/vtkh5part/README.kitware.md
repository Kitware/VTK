# H5part fork for VTK

This branch containes changes required to embed H5part into VTK.
This includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the symbols
to avoid conflicts with other copies of the library within a single process.

  * ignore whitespace issues
  * add mangling header
  * add export macros for windows support
