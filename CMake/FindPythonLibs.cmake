# - Find python libraries
# This module finds if Python is installed and determines where the
# include files and libraries are. It also determines what the name of
# the library is. This code sets the following variables:
#
#  PYTHONLIBS_FOUND           - have the Python libs been found
#  PYTHON_LIBRARIES           - path to the python library
#  PYTHON_INCLUDE_PATH        - path to where Python.h is found (deprecated)
#  PYTHON_INCLUDE_DIRS        - path to where Python.h is found
#  PYTHON_DEBUG_LIBRARIES     - path to the debug library
#  PYTHON_VERSION             - python version string e.g. 2.7.1
#  PYTHON_MAJOR_VERSION       - python major version number
#  PYTHON_MINOR_VERSION       - python minor version number
#  PYTHON_MICRO_VERSION       - python release version number
#
# This code uses the following variables:
#  Python_ADDITIONAL_VERSIONS - list of additional Python versions to search for

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# Use the executable's path as a hint
set(_Python_LIBRARY_PATH_HINT)
if(PYTHON_EXECUTABLE)
  if(WIN32)
    get_filename_component(_Python_PREFIX ${PYTHON_EXECUTABLE} PATH)
    if(_Python_PREFIX)
      set(_Python_LIBRARY_PATH_HINT ${_Python_PREFIX}/libs)
    endif()
    unset(_Python_PREFIX)
  else()
    get_filename_component(_Python_PREFIX ${PYTHON_EXECUTABLE} PATH)
    get_filename_component(_Python_PREFIX ${_Python_PREFIX} PATH)
    if(_Python_PREFIX)
      set(_Python_LIBRARY_PATH_HINT ${_Python_PREFIX}/lib)
    endif()
    unset(_Python_PREFIX)
  endif()
endif()

include(CMakeFindFrameworks)
# Search for the python framework on Apple.
CMAKE_FIND_FRAMEWORKS(Python)

# Save CMAKE_FIND_FRAMEWORK
if(DEFINED CMAKE_FIND_FRAMEWORK)
  set(_PythonLibs_CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK})
else()
  unset(_PythonLibs_CMAKE_FIND_FRAMEWORK)
endif()
# To avoid picking up the system Python.h pre-maturely.
set(CMAKE_FIND_FRAMEWORK LAST)

set(_PythonInterp_VERSION)
if(PYTHONINTERP_FOUND)
  set(_PythonInterp_VERSION ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
endif()

# Set up the versions we know about, in the order we will search. Always add
# the user supplied additional versions to the front.
set(_Python_VERSIONS
  ${Python_ADDITIONAL_VERSIONS} ${_PythonInterp_VERSION}
  2.7 2.6 2.5 3.6 3.5 3.4 3.3 3.2)

foreach(_CURRENT_VERSION ${_Python_VERSIONS})
  string(REPLACE "." "" _CURRENT_VERSION_NO_DOTS ${_CURRENT_VERSION})
  if(WIN32)
    find_library(PYTHON_DEBUG_LIBRARY
      NAMES python${_CURRENT_VERSION_NO_DOTS}_d python
      HINTS ${_Python_LIBRARY_PATH_HINT}
      PATHS
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs/Debug
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs/Debug
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
      )
  endif()

  set(PYTHON_FRAMEWORK_LIBRARIES)
  if(Python_FRAMEWORKS AND NOT PYTHON_LIBRARY)
    foreach(dir ${Python_FRAMEWORKS})
      list(APPEND PYTHON_FRAMEWORK_LIBRARIES
           ${dir}/Versions/${_CURRENT_VERSION}/lib)
    endforeach()
  endif()
  find_library(PYTHON_LIBRARY
    NAMES
      python${_CURRENT_VERSION_NO_DOTS}
      python${_CURRENT_VERSION}mu
      python${_CURRENT_VERSION}m
      python${_CURRENT_VERSION}u
      python${_CURRENT_VERSION}
    HINTS
      ${_Python_LIBRARY_PATH_HINT}
    PATHS
      ${PYTHON_FRAMEWORK_LIBRARIES}
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
    # Avoid finding the .dll in the PATH.  We want the .lib.
    NO_SYSTEM_ENVIRONMENT_PATH
  )
  # Look for the static library in the Python config directory
  find_library(PYTHON_LIBRARY
    NAMES python${_CURRENT_VERSION_NO_DOTS} python${_CURRENT_VERSION}
    # Avoid finding the .dll in the PATH.  We want the .lib.
    NO_SYSTEM_ENVIRONMENT_PATH
    # This is where the static library is usually located
    PATH_SUFFIXES python${_CURRENT_VERSION}/config
  )

  # Don't search for include dir until library location is known
  if(PYTHON_LIBRARY)

    # Use the library's install prefix as a hint
    set(_Python_INCLUDE_PATH_HINT)
    get_filename_component(_Python_PREFIX ${PYTHON_LIBRARY} PATH)
    get_filename_component(_Python_PREFIX ${_Python_PREFIX} PATH)
    if(_Python_PREFIX)
      set(_Python_INCLUDE_PATH_HINT ${_Python_PREFIX}/include)
    endif()
    unset(_Python_PREFIX)

    # Add framework directories to the search paths
    set(PYTHON_FRAMEWORK_INCLUDES)
    if(Python_FRAMEWORKS AND NOT PYTHON_INCLUDE_DIR)
      foreach(dir ${Python_FRAMEWORKS})
        list(APPEND PYTHON_FRAMEWORK_INCLUDES
          ${dir}/Versions/${_CURRENT_VERSION}/include)
      endforeach()
    endif()

    find_path(PYTHON_INCLUDE_DIR
      NAMES Python.h
      HINTS
        ${_Python_INCLUDE_PATH_HINT}
      PATHS
        ${PYTHON_FRAMEWORK_INCLUDES}
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/include
        [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/include
      PATH_SUFFIXES
        python${_CURRENT_VERSION}mu
        python${_CURRENT_VERSION}m
        python${_CURRENT_VERSION}u
        python${_CURRENT_VERSION}
    )
  endif()

  # For backward compatibility, set PYTHON_INCLUDE_PATH.
  set(PYTHON_INCLUDE_PATH "${PYTHON_INCLUDE_DIR}")

  if(PYTHON_LIBRARY AND PYTHON_INCLUDE_DIR)
    break()
  endif()
endforeach()

unset(_Python_INCLUDE_PATH_HINT)
unset(_Python_LIBRARY_PATH_HINT)

mark_as_advanced(
  PYTHON_DEBUG_LIBRARY
  PYTHON_LIBRARY
  PYTHON_INCLUDE_DIR
)


# look in PYTHON_INCLUDE_DIR for patchlevel.h, which contains the
# version number macros in all versions of python from 1.5 through
# at least version 3.2, and set these vars:  PYTHON_VERSION,
# PYTHON_MAJOR_VERSION, PYTHON_MINOR_VERSION, PYTHON_MICRO_VERSION.
IF(PYTHON_INCLUDE_DIR)
  SET(_VERSION_REGEX
      "^#define[ \t]+PY([A-Z_]*_VERSION)[ \t]+[\"]*([[0-9A-Za-z\\.]+)[\"]*[ \t]*$")
  FILE(STRINGS "${PYTHON_INCLUDE_DIR}/patchlevel.h" _VERSION_STRINGS
       LIMIT_COUNT 10 REGEX ${_VERSION_REGEX})
  FOREACH(_VERSION_STRING ${_VERSION_STRINGS})
    STRING(REGEX REPLACE ${_VERSION_REGEX} "PYTHON\\1"
           _VERSION_VARIABLE "${_VERSION_STRING}")
    STRING(REGEX REPLACE ${_VERSION_REGEX} "\\2"
           _VERSION_NUMBER "${_VERSION_STRING}")
    SET(${_VERSION_VARIABLE} ${_VERSION_NUMBER})
  ENDFOREACH()
ENDIF()


# We use PYTHON_INCLUDE_DIR, PYTHON_LIBRARY and PYTHON_DEBUG_LIBRARY for the
# cache entries because they are meant to specify the location of a single
# library. We now set the variables listed by the documentation for this
# module.
SET(PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_DIR}")
SET(PYTHON_LIBRARIES "${PYTHON_LIBRARY}")
SET(PYTHON_DEBUG_LIBRARIES "${PYTHON_DEBUG_LIBRARY}")


# Restore CMAKE_FIND_FRAMEWORK
if(DEFINED _PythonLibs_CMAKE_FIND_FRAMEWORK)
  set(CMAKE_FIND_FRAMEWORK ${_PythonLibs_CMAKE_FIND_FRAMEWORK})
  unset(_PythonLibs_CMAKE_FIND_FRAMEWORK)
else()
  unset(CMAKE_FIND_FRAMEWORK)
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PythonLibs DEFAULT_MSG PYTHON_LIBRARIES PYTHON_INCLUDE_DIRS)
