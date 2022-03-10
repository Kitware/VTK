# Note that OpenXR lacks a useful install tree. This should work if
# `OpenXR_ROOT` is set to the source directory of OpenXR.

# TODO: fails for universal builds
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_openxr_bitness 64)
else ()
  set(_openxr_bitness 32)
endif ()

set(_openxr_platform_base)
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(_openxr_platform_base osx)
  # SteamVR only supports 32-bit on OS X
  set(OpenXR_PLATFORM osx32)
else ()
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(_openxr_platform_base linux)
  elseif (WIN32)
    set(_openxr_platform_base win)
  endif ()
  set(OpenXR_PLATFORM ${_openxr_platform_base}${_openxr_bitness})
endif ()

find_path(OpenXR_INCLUDE_DIR
  NAMES
    openxr.h
    openxr_platform.h
  PATH_SUFFIXES
    openxr
    headers
    public/headers
    steam
    public/steam
  DOC "OpenXR include directory")
mark_as_advanced(OpenXR_INCLUDE_DIR)

find_library(OpenXR_LIBRARY
  NAMES openxr_loader
  PATH_SUFFIXES
    "${OpenXR_PLATFORM}"
    "bin/${OpenXR_PLATFORM}"
  DOC "OpenXR API library")
mark_as_advanced(OpenXR_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenXR
  REQUIRED_VARS OpenXR_LIBRARY OpenXR_INCLUDE_DIR)

if (OpenXR_FOUND)
  set(OpenXR_INCLUDE_DIRS "${OpenXR_INCLUDE_DIR}")
  set(OpenXR_LIBRARIES "${OpenXR_LIBRARY}")
  if (NOT TARGET OpenXR::OpenXR)
    add_library(OpenXR::OpenXR UNKNOWN IMPORTED)
    set_target_properties(OpenXR::OpenXR PROPERTIES
      IMPORTED_LOCATION "${OpenXR_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${OpenXR_INCLUDE_DIR}")
  endif ()
endif ()

unset(_openxr_bitness)
unset(_openxr_platform_base)

# See https://github.com/KhronosGroup/OpenXR-Hpp/blob/master/cmake/FindOpenXR.cmake
