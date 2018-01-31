# libharu fork for VTK

This branch contains changes required to embed libharu into VTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Add .gitattributes file to ignore whitespace from commit checks.
  * Remove invalid UTF8 characters.
  * Integrate with VTK's module system.
  * Mangle symbols to start with a `vtklibharu_` prefix.
