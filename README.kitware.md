# KISSFFT for VTK

This branch contains changes required to embed kissfft library into VTK. This
includes the following changes.

1. remove source files and directories not needed.
2. add CMakeLists.txt for build/install rules (note, unlike upstream,
   we are packaging headers/sources from `tools` subdir in the same library.
3. add `kiss_fft_exports.h` to handle symbol exports
4. added `vtk_kissfft_mangle.h` header for mangling exported symbols.
