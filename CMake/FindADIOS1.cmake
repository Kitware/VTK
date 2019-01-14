# - Find ADIOS 1.x library, routines for scientific, parallel IO
#   https://www.olcf.ornl.gov/center-projects/adios/
#
# Use this module by invoking find_package with the form:
#   find_package(ADIOS1
#     [version] [EXACT]     # Minimum or EXACT version, e.g. 1.6.0
#     [REQUIRED]            # Fail with an error if ADIOS or a required
#                           #   component is not found
#     [QUIET]               # ...
#     [COMPONENTS <...>]    # Compiled in components: fortran, readonly,
                            # sequential (all are case insentative)
#   )
#
# Module that finds the includes and libraries for a working ADIOS 1.x install.
# This module invokes the `adios_config` script that should be installed with
# the other ADIOS tools.
#
# To provide a hint to the module where to find the ADIOS installation,
# set the ADIOS1_ROOT environment variable.
#
# If this variable is not set, make sure that at least the according `bin/`
# directory of ADIOS 1.x is in your PATH environment variable.
#
# Set the following CMake variables BEFORE calling find_packages to
# influence this module:
#   ADIOS1_USE_STATIC_LIBS - Set to ON to force the use of static
#                           libraries.  Default: OFF
#
# This module will define the following variables:
#   ADIOS1_INCLUDE_DIRS    - Include directories for the ADIOS 1.x headers.
#   ADIOS1_LIBRARY_PATH    - Full path of the libadios library (.a or .so file)
#   ADIOS1_DEPENDENCY_LIBRARIES       - ADIOS 1.x dependency libraries.
#   ADIOS1_FOUND           - TRUE if FindADIOS1 found a working install
#   ADIOS1_VERSION         - Version in format Major.Minor.Patch
#
# Not used for now:
#   ADIOS1_DEFINITIONS     - Compiler definitions you should add with
#                           add_definitions(${ADIOS1_DEFINITIONS})
#
# Example to find ADIOS 1.x (default)
# find_package(ADIOS1)
# if(ADIOS1_FOUND)
#   include_directories(${ADIOS1_INCLUDE_DIRS})
#   add_executable(foo foo.c)
#   target_link_libraries(foo ${ADIOS1_LIBRARY_PATH} ADIOS1_DEPENDENCY_LIBRARIES)
# endif()

# Example to find ADIOS 1.x using component
# find_package(ADIOS1 COMPONENTS fortran)
# if(ADIOS1_FOUND)
#   include_directories(${ADIOS1_INCLUDE_DIRS})
#   add_executable(foo foo.c)
#   target_link_libraries(foo ${ADIOS1_LIBRARY_PATH} ${ADIOS1_DEPENDENCY_LIBRARIES})
# endif()
###############################################################################
#Copyright (c) 2014, Axel Huebl and Felix Schmitt from http://picongpu.hzdr.de
#All rights reserved.

#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:

#1. Redistributions of source code must retain the above copyright notice, this
#list of conditions and the following disclaimer.

#2. Redistributions in binary form must reproduce the above copyright notice,
#this list of conditions and the following disclaimer in the documentation
#and/or other materials provided with the distribution.

#3. Neither the name of the copyright holder nor the names of its contributors
#may be used to endorse or promote products derived from this software without
#specific prior written permission.

#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
###############################################################################


###############################################################################
# ADIOS
###############################################################################
# get flags for adios_config, -l is the default
#-f for fortran, -r for readonly, -s for sequential (nompi)
if(ADIOS1_FIND_COMPONENTS)
  foreach(comp IN LISTS ADIOS1_FIND_COMPONENTS)
    string(TOLOWER "${comp}" comp)
    if(comp STREQUAL "fortran")
      set(adios1_config_opt "-f")
    elseif(comp STREQUAL "readonly")
      set(adios1_config_opt "-r")
    elseif(comp STREQUAL "sequential")
      set(adios1_config_opt "-s")
    else()
      message("ADIOS 1.x component ${comp} is not supported. Please use fortran, readonly, or sequential")
    endif()
  endforeach()
endif()

set(adios1_config_hints)
foreach(PREFIX_VAR IN ITEMS ADIOS1_ROOT INSTALL_PREFIX)
  if(${PREFIX_VAR})
    list(APPEND adios1_config_hints "${${PREFIX_VAR}}/bin")
  elseif(NOT ("$ENV{${PREFIX_VAR}}" STREQUAL ""))
    list(APPEND adios1_config_hints "$ENV{${PREFIX_VAR}}/bin")
  endif()
endforeach()

# find `adios_config` program #################################################
find_program(ADIOS1_CONFIG NAME adios_config HINTS ${adios1_config_hints})

# check `adios_config` program ################################################
if(ADIOS1_CONFIG)
  execute_process(COMMAND ${ADIOS1_CONFIG} -c ${adios1_config_opt}
    OUTPUT_VARIABLE adios1_config_out
    RESULT_VARIABLE adios1_config_ret
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(adios1_config_ret EQUAL 0)
    string(REPLACE " " ";" adios1_match "${adios1_config_out}")
    set(adios1_include_hints)
    set(ADIOS1_COMPILE_OPTIONS)
    foreach(OPT IN LISTS adios1_match)
      if(OPT MATCHES "^-I(.*)")
        list(APPEND adios1_include_hints "${CMAKE_MATCH_1}")
      else()
        list(APPEND ADIOS1_COMPILE_OPTIONS ${OPT})
      endif()
    endforeach()
  endif()

  execute_process(COMMAND ${ADIOS1_CONFIG} -l ${adios1_config_opt}
    OUTPUT_VARIABLE adios1_config_out
    RESULT_VARIABLE adios1_config_ret
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(adios1_config_ret EQUAL 0)
    string(REPLACE " " ";" adios1_match "${adios1_config_out}")
    set(adios1_libs)
    set(adios1_lib_hints)
    set(adios1_lib_flags)
    foreach(OPT IN LISTS adios1_match)
      if(OPT MATCHES "^-L(.*)")
        list(APPEND adios1_lib_hints "${CMAKE_MATCH_1}")
      elseif(OPT MATCHES "^-l(.*)")
        list(APPEND adios1_libs "${CMAKE_MATCH_1}")
      else()
        list(APPEND adios1_lib_flags "${OPT}")
      endif()
    endforeach()
  endif()

  # add the version string
  execute_process(COMMAND ${ADIOS1_CONFIG} -v
    OUTPUT_VARIABLE ADIOS1_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

# make suree at the very least we find libadios
if(NOT adios1_libs)
  set(adios1_libs adios)
endif()

# Search for the actual libs and headers to ue based on hints from the config
find_path(ADIOS1_INCLUDE_DIR adios.h HINTS ${adios1_include_hints})

set(ADIOS1_LIBRARY)
set(ADIOS1_DEPENDENCIES)
foreach(lib IN LISTS adios1_libs)
  find_library(ADIOS1_${lib}_LIBRARY NAME ${lib} HINTS ${adios1_lib_hints})
  if(ADIOS1_${lib}_LIBRARY)
    if(lib MATCHES "^adios")
      set(ADIOS1_LIBRARY ${ADIOS1_${lib}_LIBRARY})
    else()
      list(APPEND ADIOS1_DEPENDENCIES ${ADIOS1_${lib}_LIBRARY})
    endif()
  else()
    list(APPEND ADIOS1_DEPENDENCIES ${lib})
  endif()
endforeach()

find_package(Threads REQUIRED)
list(APPEND ADIOS1_DEPENDENCIES ${adios1_lib_flags} ${CMAKE_THREAD_LIBS_INIT})

###############################################################################
# FindPackage Options
###############################################################################

# handles the REQUIRED, QUIET and version-related arguments for find_package
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ADIOS1
  REQUIRED_VARS ADIOS1_LIBRARY ADIOS1_INCLUDE_DIR
  VERSION_VAR ADIOS1_VERSION
)

if(ADIOS1_FOUND)
  set(ADIOS1_INCLUDE_DIRS ${ADIOS1_INCLUDE_DIR})
  set(ADIOS1_LIBRARIES ${ADIOS1_LIBRARY} ${ADIOS1_DEPENDENCIES})

  ##########################################################################
  # Add target and dependencies to ADIOS2
  ##########################################################################
  if(NOT TARGET adios1::adios)
    add_library(adios1::adios UNKNOWN IMPORTED)
    set_target_properties(adios1::adios PROPERTIES
      IMPORTED_LOCATION "${ADIOS1_LIBRARY}"
      INTERFACE_LINK_LIBRARIES "${ADIOS1_DEPENDENCIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${ADIOS1_INCLUDE_DIRS}"
      INTERFACE_COMPILE_OPTIONS "${ADIOS1_COMPILE_OPTIONS}"
    )
  endif()
endif()
