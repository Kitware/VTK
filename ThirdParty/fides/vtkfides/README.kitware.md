# Fides fork for VTK

This branch contains changes required to embed Fides into VTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree as well as a header to facilitate mangling of the symbols
to avoid conflicts with other copies of the library within a single process.

  * avoid using `clang-format` for the import
  * use VTK's MPI
  * update CMake files to use `vtk_module` to get into VTK's install tree
