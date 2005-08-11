MESSAGE(FATAL_ERROR
  "ERROR: VTK installation is disabled because VTK_USE_RPATH is ON.  It is not safe to install binaries that have a runtime path pointing to the build tree.  In order to install safely you need to delete the libraries from your build tree, set VTK_USE_RPATH to OFF, and rebuild.  See comments in VTK/CMake/vtkSelectSharedLibraries.cmake for more information."
)
