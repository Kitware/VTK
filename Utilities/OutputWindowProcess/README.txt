In order to build this project just configure it with CMake as a
separate project.  After running the "Configure" step, there will be a
vtkWin32OutputWindowProcessEncoded.c at the top level of the build
tree.  There is no need to actually load and build the project with
Visual Studio.

This project is intended to generate
vtkWin32OutputWindowProcessEncoded.c for inclusion in the build of
vtkCommon.  The executable is self-deleting and is used by
vtkWin32ProcessOutputWindow.  It is an output window that runs as a
separate process and deletes its own executable on exit.  This is
useful so that if the main process crashes, the output window is still
usable, which is good since it probably explains the crash.

Currently the self-deletion mechanism works on all versions of windows
but only when compiled by a Visual Studio compiler in release mode.

If vtkWin32OutputWindowProcess.c can be implemented in a way that
works for all windows compilers, then this project can be integrated
into the main VTK build process by adding a custom command to generate
vtkWin32OutputWindowProcessEncoded.c on the fly like this:

IF(WIN32)
  IF (NOT VTK_USE_X)
    SET(VTK_OWP_ENCODED_C
      ${VTK_BINARY_DIR}/Common/vtkWin32OutputWindowProcessEncoded.c)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${VTK_OWP_ENCODED_C}
      COMMAND ${CMAKE_COMMAND}
      ARGS -G\"${CMAKE_GENERATOR}\"
           -H${VTK_SOURCE_DIR}/Utilities/OutputWindowProcess
           -B${VTK_BINARY_DIR}/Utilities/OutputWindowProcess
           -DVTK_OWP_OUTPUT=${VTK_OWP_ENCODED_C}
      DEPENDS ${VTK_SOURCE_DIR}/Utilities/OutputWindowProcess/vtkWin32OutputWindowProcess.c
      )
    SET_SOURCE_FILES_PROPERTIES(${VTK_OWP_ENCODED_C} PROPERTIES WRAP_EXCLUDE 1)
  ENDIF (NOT VTK_USE_X)
ENDIF(WIN32)
