# Determine if X11 should be included.
# This provides a option to the user to not use X11 on systems where
# applicable. If systems where X11 is found and is not optional,
# it does not provide a CMake option instead the variable VTK_USE_X
# if set to ON.
FIND_PACKAGE(X11)
SET(VTK_USE_X_OPTIONAL 0)
SET(VTK_USE_X_DEFAULT 0)
IF(X11_FOUND)
  IF(CYGWIN)
    SET(VTK_USE_X_OPTIONAL 1)
  ENDIF()
  IF(APPLE)
    SET(VTK_USE_X_OPTIONAL 1)
  ELSE()
    IF (VTK_OPENGL_HAS_OSMESA)
      SET(VTK_USE_X_OPTIONAL 1)
    ENDIF ()
    SET(VTK_USE_X_DEFAULT ${VTK_USE_RENDERING})
  ENDIF()
ENDIF()

VTK_DEPENDENT_OPTION(VTK_USE_X
                     "Build classes for the X11 window system."
                     "${VTK_USE_X_DEFAULT}"
                     "X11_FOUND;VTK_USE_RENDERING;VTK_USE_X_OPTIONAL"
                     "${VTK_USE_X_DEFAULT}")
