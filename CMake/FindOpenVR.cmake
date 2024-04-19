# Note that OpenVR lacks a useful install tree. This should work if
# `OpenVR_ROOT` is set to the source directory of OpenVR.

# TODO: fails for universal builds
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_openvr_bitness 64)
else ()
  set(_openvr_bitness 32)
endif ()

set(_openvr_platform_base)
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(_openvr_platform_base osx)
  # SteamVR only supports 32-bit on OS X
  set(OpenVR_PLATFORM osx32)
else ()
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(_openvr_platform_base linux)
  elseif (WIN32)
    set(_openvr_platform_base win)
  endif ()
  set(OpenVR_PLATFORM ${_openvr_platform_base}${_openvr_bitness})
endif ()

find_path(OpenVR_INCLUDE_DIR
  NAMES
    openvr.h
  PATH_SUFFIXES
    openvr
    headers
    public/headers
    steam
    public/steam
  DOC "OpenVR include directory")
mark_as_advanced(OpenVR_INCLUDE_DIR)

find_library(OpenVR_LIBRARY
  NAMES openvr_api64 openvr_api
  PATH_SUFFIXES
    "${OpenVR_PLATFORM}"
    "bin/${OpenVR_PLATFORM}"
  DOC "OpenVR API library")
mark_as_advanced(OpenVR_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenVR
  REQUIRED_VARS OpenVR_LIBRARY OpenVR_INCLUDE_DIR)

if (OpenVR_FOUND)
  set(OpenVR_INCLUDE_DIRS "${OpenVR_INCLUDE_DIR}")
  set(OpenVR_LIBRARIES "${OpenVR_LIBRARY}")
  if (NOT TARGET OpenVR::OpenVR)
    add_library(OpenVR::OpenVR UNKNOWN IMPORTED)
    set_target_properties(OpenVR::OpenVR PROPERTIES
      IMPORTED_LOCATION "${OpenVR_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${OpenVR_INCLUDE_DIR}")
  endif ()
endif ()

unset(_openvr_bitness)
unset(_openvr_platform_base)
