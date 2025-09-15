# VTK version number components.
set(VTK_MAJOR_VERSION 9)
set(VTK_MINOR_VERSION 5)

# Git conflict avoidance barrier. When branching for an `X.Y.0.rc1` branch, if
# the date component is updated while it is in progress, the bump of the minor
# version ends up conflicting with the patch version update.

set(VTK_BUILD_VERSION 2)

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
