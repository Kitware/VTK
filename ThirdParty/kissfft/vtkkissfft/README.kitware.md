# kissfft fork for VTK

This branch contains changes required to embed kissfft into VTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree as well as a header to facilitate mangling of the symbols
to avoid conflicts with other copies of the library within a single process.

  * ignore whitespace errors
  * integrate with VTK's module system
  * mangle symbols
  * export symbols
