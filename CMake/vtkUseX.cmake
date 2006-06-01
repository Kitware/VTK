# Determine if X11 should be included.
# This provides a option to the user to not use X11 on systems where
# applicable. If systems where X11 is found and is not optional,
# it does not provide a CMake option instead the variable VTK_USE_X
# if set to ON.
FIND_PACKAGE(X11)
SET(VTK_USE_X_OPTIONAL 0)
SET(VTK_USE_X_FORCE 0)
IF(X11_FOUND)
  IF(CYGWIN)
    SET(VTK_USE_X_OPTIONAL 1)
  ENDIF(CYGWIN)
  IF(APPLE)
    SET(VTK_USE_X_OPTIONAL 1)
  ELSE(APPLE)
    SET(VTK_USE_X_FORCE ${VTK_USE_RENDERING})
  ENDIF(APPLE)
ENDIF(X11_FOUND)

VTK_DEPENDENT_OPTION(VTK_USE_X
                     "Build classes for the X11 window system." OFF
                     "X11_FOUND;VTK_USE_RENDERING;VTK_USE_X_OPTIONAL"
                     "${VTK_USE_X_FORCE}")
