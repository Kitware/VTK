# PROJ fork for VTK

This branch contains changes required to embed libproj into VTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Ignore whitespace errors for VTK's commit checks.
  * Integrate the CMake build with VTK's module system.
  * Mangle all exported symbols to have a `vtklibproj_` prefix.
  * On emscripten hosts, generate `proj.db` with `sqlitebin -init filename` instead of relying on `stdin`.
  * Add `-Wno-experimental` for `wasm64-unknown` architecture.
