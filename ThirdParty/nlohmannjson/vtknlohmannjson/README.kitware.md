# nlohmann\_json fork for VTK

This branch contains changes required to embed nlohmann\_json into VTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Ignore whitespace for VTK's commit checks
  * Update CMakeLists.vtk.txt to include build/install rules for VTK's module
    system.
  * Mangle nlohmann namespace.
