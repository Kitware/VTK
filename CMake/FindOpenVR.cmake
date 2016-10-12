# - try to find the OpenVR SDK - currently designed for the version on GitHub.
#
# Cache Variables: (probably not for direct use in your scripts)
#  OPENVR_INCLUDE_DIR
#
# Non-cache variables you might use in your CMakeLists.txt:
#  OPENVR_FOUND
#  OPENVR_INCLUDE_DIRS
#  OPENVR_PLATFORM - something like Win32, Win64, etc.
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Original Author:
# 2015 Ryan A. Pavlik <ryan@sensics.com>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set(OPENVR_ROOT_DIR
  "${OPENVR_ROOT_DIR}"
  CACHE
  PATH
  "Directory to search for OpenVR SDK")

set(OPENVR_HEADERS_ROOT_DIR
  "${OPENVR_HEADERS_ROOT_DIR}"
  CACHE
  PATH
  "Directory to search for private OpenVR headers")

set(_root_dirs)
if(OPENVR_ROOT_DIR)
  set(_root_dirs "${OPENVR_ROOT_DIR}" "${OPENVR_HEADERS_ROOT_DIR}" "${OPENVR_ROOT_DIR}/public")
endif()

# todo fails for universal builds
set(_dll_suffix)
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
  set(_bitness 64)
  if(WIN32)
    set(_dll_suffix _x64)
  endif()
else()
  set(_bitness 32)
endif()

# Test platform

set(_platform)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(_platform_base osx)
  # SteamVR only supports 32-bit on OS X
  set(OPENVR_PLATFORM osx32)
else()
  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(_platform_base linux)
    # TODO Massive hack!
    add_definitions(-DGNUC -DPOSIX -DCOMPILER_GCC -D_LINUX -DLINUX -DPOSIX -D_POSIX)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(_platform_base win)
  endif()
  set(OPENVR_PLATFORM ${_platform_base}${_bitness})
  set(_libpath lib/${OPENVR_PLATFORM})
endif()

find_path(OPENVR_INCLUDE_DIR
  NAMES
  openvr_driver.h
  HINTS
  "${_libdir}"
  "${_libdir}/.."
  "${_libdir}/../.."
  PATHS
  ${_root_dirs}
  PATH_SUFFIXES
  headers
  public/headers
  steam
  public/steam)

FIND_LIBRARY(OPENVR_LIBRARY_TEMP
  NAMES openvr_api
  HINTS
  PATH_SUFFIXES ${_libpath}
  PATHS ${OPENVR_ROOT_DIR}
)

IF (OPENVR_LIBRARY_TEMP)
  # Set the final string here so the GUI reflects the final state.
  SET(OPENVR_LIBRARY ${OPENVR_LIBRARY_TEMP} CACHE STRING "Where the openvr Library can be found")
  # Set the temp variable to INTERNAL so it is not seen in the CMake GUI
  SET(OPENVR_LIBRARY_TEMP "${OPENVR_LIBRARY_TEMP}" CACHE INTERNAL "")
ENDIF(OPENVR_LIBRARY_TEMP)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenVR
  DEFAULT_MSG
  OPENVR_INCLUDE_DIR OPENVR_LIBRARY)

if(OPENVR_FOUND)
  list(APPEND OPENVR_INCLUDE_DIRS ${OPENVR_INCLUDE_DIR})
  mark_as_advanced(OPENVR_ROOT_DIR)
endif()

mark_as_advanced(OPENVR_INCLUDE_DIR)