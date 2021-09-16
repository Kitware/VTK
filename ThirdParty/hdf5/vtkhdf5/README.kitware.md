# HDF5 fork for VTK

This branch contains changes required to embed HDF5 into VTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree as well as a header to facilitate mangling of the symbols
to avoid conflicts with other copies of the library within a single process.

  * Fix CMake to work within VTK's module system. This largely involves
    hard-coding some settings to reduce the number of cache entries exposed.
