# VTK version number components.
set(VTK_MAJOR_VERSION 9)
set(VTK_MINOR_VERSION 0)
set(VTK_BUILD_VERSION 0)

if (NOT VTK_MINOR_VERSION LESS 100)
  message(FATAL_ERROR
    "The minor version number cannot exceed 100 without changing "
    "`VTK_VERSION_CHECK`.")
endif ()

if (NOT VTK_BUILD_VERSION LESS 100000000)
  message(FATAL_ERROR
    "The build version number cannot exceed 100000000 without changing "
    "`VTK_VERSION_CHECK`.")
endif ()
