# Include this file from a CMakeLists.txt that contains an
# VTK_ADD_EXECUTABLE or VTK_ADD_LIBRARY call when the build product
# links against MFC.
#
# This file provides definitions for the following CMake vars:
#   VTK_MFC_STATIC
#   VTK_MFC_LIB_TYPE
#   VTK_MFC_DELAYLOAD_VTK_DLLS
#   VTK_MFC_EXTRA_LIBS
#
# It also automatically sets CMAKE_MFC_FLAG based on VTK_MFC_STATIC
# and adds the _AFXDLL definition when linking to MFC dll.
#
# After including this file, use something like this chunk:
#   IF(VTK_MFC_DELAYLOAD_VTK_DLLS)
#     VTK_MFC_ADD_DELAYLOAD_FLAGS(CMAKE_EXE_LINKER_FLAGS
#       vtkRendering.dll vtkFiltering.dll vtkCommon.dll
#     )
#   ENDIF(VTK_MFC_DELAYLOAD_VTK_DLLS)
# And then after your ADD_EXECUTABLE or ADD_LIBRARY:
#   IF(VTK_MFC_EXTRA_LIBS)
#     TARGET_LINK_LIBRARIES(yourLibOrExe ${VTK_MFC_EXTRA_LIBS})
#   ENDIF(VTK_MFC_EXTRA_LIBS)
#
# If you're building an exe, use CMAKE_EXE_LINKER_FLAGS...
# If building a dll, use CMAKE_SHARED_LINKER_FLAGS instead...


# C runtime lib linkage and MFC lib linkage *MUST* match.
# If linking to C runtime static lib, link to MFC static lib.
# If linking to C runtime dll, link to MFC dll.
#
# If "/MT" or "/MTd" is in the compiler flags, then our build
# products will be linked to the C runtime static lib. Otherwise,
# to the C runtime dll.
#
IF(NOT DEFINED VTK_MFC_STATIC)
  SET(VTK_MFC_STATIC OFF)
  IF("${CMAKE_CXX_FLAGS_RELEASE}" MATCHES "/MT")
    SET(VTK_MFC_STATIC ON)
  ENDIF("${CMAKE_CXX_FLAGS_RELEASE}" MATCHES "/MT")
ENDIF(NOT DEFINED VTK_MFC_STATIC)

IF(VTK_MFC_STATIC)
  # Tell CMake to link to MFC static lib
  SET(VTK_MFC_LIB_TYPE "STATIC")
  SET(CMAKE_MFC_FLAG 1)
ELSE(VTK_MFC_STATIC)
  # Tell CMake to link to MFC dll
  SET(VTK_MFC_LIB_TYPE "SHARED")
  SET(CMAKE_MFC_FLAG 2)
  ADD_DEFINITIONS(-D_AFXDLL)
ENDIF(VTK_MFC_STATIC)


# Using the linker /DELAYLOAD flag is necessary when VTK is built
# as dlls to avoid false mem leak reporting by MFC shutdown code.
# Without it, the VTK dlls load before the MFC dll at startup and
# unload after the MFC dll unloads at shutdown. Hence, any VTK objects
# left at MFC dll unload time get reported as leaks.
#
IF(NOT DEFINED VTK_MFC_DELAYLOAD_VTK_DLLS)
  SET(VTK_MFC_DELAYLOAD_VTK_DLLS OFF)
  IF(VTK_BUILD_SHARED_LIBS)
    SET(VTK_MFC_DELAYLOAD_VTK_DLLS ON)
  ENDIF(VTK_BUILD_SHARED_LIBS)
ENDIF(NOT DEFINED VTK_MFC_DELAYLOAD_VTK_DLLS)

IF(NOT DEFINED VTK_MFC_EXTRA_LIBS)
  SET(VTK_MFC_EXTRA_LIBS)
ENDIF(NOT DEFINED VTK_MFC_EXTRA_LIBS)

IF(VTK_MFC_STATIC)
  SET(VTK_MFC_EXTRA_LIBS ${VTK_MFC_EXTRA_LIBS}
    debug nafxcwd optimized nafxcw
    debug LIBCMTD optimized LIBCMT
    )
ENDIF(VTK_MFC_STATIC)

IF(VTK_MFC_DELAYLOAD_VTK_DLLS)
  SET(VTK_MFC_EXTRA_LIBS ${VTK_MFC_EXTRA_LIBS} "DelayImp")
ENDIF(VTK_MFC_DELAYLOAD_VTK_DLLS)

MACRO(VTK_MFC_ADD_DELAYLOAD_FLAGS flagsVar)
  SET(dlls "${ARGN}")
  FOREACH(dll ${dlls})
    SET(${flagsVar} "${${flagsVar}} /DELAYLOAD:${dll}")
  ENDFOREACH(dll)
ENDMACRO(VTK_MFC_ADD_DELAYLOAD_FLAGS)
