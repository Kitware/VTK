# - try to find the Oculus SDK - currently designed for the version 1.6
#
# Cache Variables: (probably not for direct use in your scripts)
#  OCULUS_ROOT
#
# Non-cache variables you might use in your CMakeLists.txt:
#  OCULUS_FOUND
#  OCULUS_INCLUDE_DIRS
#  OCULUS_LIBS
#  OCULUS_PLATFORM - something like Win32, x64, etc.
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#

set(OCULUS_ROOT_DIR
  "${OCULUS_ROOT_DIR}"
  CACHE
  PATH
  "Directory to search for Oculus SDK")

set(_root_dirs)
if(OCULUS_ROOT_DIR)
  set(_root_dirs "${OCULUS_ROOT_DIR}")
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
  set(OCULUS_PLATFORM osx32)
else()
  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(_platform_base linux)
    set(OCULUS_PLATFORM ${_platform_base})
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(_platform_base Windows)
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
      set(OCULUS_PLATFORM ${_platform_base}/x64)
    else ()
      set(OCULUS_PLATFORM ${_platform_base}/Win32)
    endif()
    set(_vs_version VS2012)
    if (NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 17.0)
      set(_vs_version VS2012)
    endif()
    if (NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 18.0)
      set(_vs_version VS2013)
    endif()
    if (NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 19.0)
      set(_vs_version VS2015)
    endif()
  endif()
  set(_libpath Lib/${OCULUS_PLATFORM})
endif()

find_path(OCULUS_INCLUDE_DIR
  NAMES
    OVR_Version.h
  HINTS
  PATHS
    ${_root_dirs}
  PATH_SUFFIXES
    LibOVR
    LibOVR/Include)

FIND_LIBRARY(OCULUS_LIBRARY
  NAMES LibOVR.lib
  HINTS
  PATH_SUFFIXES
    ${_libpath}
    ${_libpath}/Release/${_vs_version}
  PATHS
   ${OCULUS_ROOT_DIR}/LibOVR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OCULUS
  DEFAULT_MSG
  OCULUS_INCLUDE_DIR OCULUS_LIBRARY)

if(OCULUS_FOUND)
  set (OCULUS_LIBRARIES ${OCULUS_LIBRARY})
  list(APPEND OCULUS_INCLUDE_DIRS ${OCULUS_INCLUDE_DIR})
endif()

mark_as_advanced(OCULUS_LIBRARY OCULUS_INCLUDE_DIR)