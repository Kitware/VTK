# - Find ADIOS library, routines for scientific, parallel IO
#   https://www.olcf.ornl.gov/center-projects/adios/
#
# Use this module by invoking find_package with the form:
#   find_package(ADIOS
#     [version] [EXACT]     # Minimum or EXACT version, e.g. 1.6.0
#     [REQUIRED]            # Fail with an error if ADIOS or a required
#                           #   component is not found
#     [QUIET]               # ...
#     [COMPONENTS <...>]    # Compiled in components: fortran, readonly,
                            # sequential (all are case insentative)
#   )
#
# Module that finds the includes and libraries for a working ADIOS install.
# This module invokes the `adios_config` script that should be installed with
# the other ADIOS tools.
#
# To provide a hint to the module where to find the ADIOS installation,
# set the ADIOS_ROOT environment variable.
#
# If this variable is not set, make sure that at least the according `bin/`
# directory of ADIOS is in your PATH environment variable.
#
# Set the following CMake variables BEFORE calling find_packages to
# influence this module:
#   ADIOS_USE_STATIC_LIBS - Set to ON to force the use of static
#                           libraries.  Default: OFF
#
# This module will define the following variables:
#   ADIOS_INCLUDE_DIRS    - Include directories for the ADIOS headers.
#   ADIOS_LIBRARIES       - ADIOS libraries.
#   ADIOS_FOUND           - TRUE if FindADIOS found a working install
#   ADIOS_VERSION         - Version in format Major.Minor.Patch
#
# Not used for now:
#   ADIOS_DEFINITIONS     - Compiler definitions you should add with
#                           add_definitions(${ADIOS_DEFINITIONS})
#
# Example to find ADIOS (default)
# find_package(ADIOS)
# if(ADIOS_FOUND)
#   include_directories(${ADIOS_INCLUDE_DIRS})
#   add_executable(foo foo.c)
#   target_link_libraries(foo ${ADIOS_LIBRARIES})
# endif()

# Example to find ADIOS using component
# find_package(ADIOS COMPONENTS fortran)
# if(ADIOS_FOUND)
#   include_directories(${ADIOS_INCLUDE_DIRS})
#   add_executable(foo foo.c)
#   target_link_libraries(foo ${ADIOS_LIBRARIES})
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
# Required cmake version
###############################################################################

cmake_minimum_required(VERSION 2.8.5)


###############################################################################
# ADIOS
###############################################################################
# get flags for adios_config, -l is the default
#-f for fortran, -r for readonly, -s for sequential (nompi)
set(OPTLIST "-l")
if(ADIOS_FIND_COMPONENTS)
    foreach(COMP ${ADIOS_FIND_COMPONENTS})
        string(TOLOWER ${COMP} comp)
        if(comp STREQUAL "fortran")
            set(OPTLIST ${OPTLIST} f)
        elseif(comp STREQUAL "readonly")
            set(OPTLIST ${OPTLIST} r)
        elseif(comp STREQUAL "sequential")
            set(OPTLIST ${OPTLIST} s)
        else()
            message("ADIOS component ${COMP} is not supported. Please use fortran, readonly, or sequential")
        endif()
    endforeach()
endif()

# we start by assuming we found ADIOS and falsify it if some
# dependencies are missing (or if we did not find ADIOS at all)
set(ADIOS_FOUND TRUE)


# find `adios_config` program #################################################
#   check the ADIOS_ROOT hint and the normal PATH
find_file(ADIOS_CONFIG
    NAME adios_config
    PATHS $ENV{ADIOS_ROOT}/bin $ENV{ADIOS_DIR}/bin $ENV{INSTALL_PREFIX}/bin $ENV{PATH})

if(ADIOS_CONFIG)
    message(STATUS "Found 'adios_config': ${ADIOS_CONFIG}")
else(ADIOS_CONFIG)
    set(ADIOS_FOUND FALSE)
    message(STATUS "Can NOT find 'adios_config' - set ADIOS_ROOT, ADIOS_DIR or INSTALL_PREFIX, or check your PATH")
endif(ADIOS_CONFIG)

# check `adios_config` program ################################################
if(ADIOS_FOUND)
    execute_process(COMMAND ${ADIOS_CONFIG} ${OPTLIST}
                    OUTPUT_VARIABLE ADIOS_LINKFLAGS
                    RESULT_VARIABLE ADIOS_CONFIG_RETURN
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT ADIOS_CONFIG_RETURN EQUAL 0)
        set(ADIOS_FOUND FALSE)
        message(STATUS "Can NOT execute 'adios_config' - check file permissions")
    endif()

    # find ADIOS_ROOT_DIR
    execute_process(COMMAND ${ADIOS_CONFIG} -d
                    OUTPUT_VARIABLE ADIOS_ROOT_DIR
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT IS_DIRECTORY "${ADIOS_ROOT_DIR}")
        set(ADIOS_FOUND FALSE)
        message(STATUS "The directory provided by 'adios_config -d' does not exist: ${ADIOS_ROOT_DIR}")
    endif()
endif(ADIOS_FOUND)

# option: use only static libs ################################################
if(ADIOS_USE_STATIC_LIBS)
    # carfully: we have to restore the original path in the end
    set(_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()


# we found something in ADIOS_ROOT_DIR and adios_config works #################
if(ADIOS_FOUND)
    # ADIOS headers
    list(APPEND ADIOS_INCLUDE_DIRS ${ADIOS_ROOT_DIR}/include)

    # check for compiled in dependencies, recomve ";" in ADIOS_LINKFLAGS (from cmake build)
    string(REGEX REPLACE ";" " " ADIOS_LINKFLAGS "${ADIOS_LINKFLAGS}")
    message(STATUS "ADIOS linker flags (unparsed): ${ADIOS_LINKFLAGS}")

    # find all library paths -L
    #   note: this can cause trouble if some libs are specified twice from
    #         different sources (quite unlikely)
    #         http://www.cmake.org/pipermail/cmake/2008-November/025128.html
    set(ADIOS_LIBRARY_DIRS "")
    string(REGEX MATCHALL "-L([A-Za-z_0-9/\\.-]+)" _ADIOS_LIBDIRS "${ADIOS_LINKFLAGS}")
    foreach(_LIBDIR ${_ADIOS_LIBDIRS})
        string(REPLACE "-L" "" _LIBDIR ${_LIBDIR})
        list(APPEND ADIOS_LIBRARY_DIRS ${_LIBDIR})
    endforeach()
    # we could append ${CMAKE_PREFIX_PATH} now but that is not really necessary

    #message(STATUS "ADIOS DIRS to look for libs: ${ADIOS_LIBRARY_DIRS}")

    # parse all -lname libraries and find an absolute path for them. we can't just
    # look for "-l" because the following library /usr/lib/x86_64-linux-gnu/libm.a
    # matches that but isn't what is looked for. First we look for " -l" and later
    # look for "-l" at the beginning of a line.
    string(REGEX MATCHALL " -l([A-Za-z_0-9\\.-]+)" _ADIOS_LIBS "${ADIOS_LINKFLAGS}")
    foreach(_LIB ${_ADIOS_LIBS})
        string(REPLACE " -l" "" _LIB ${_LIB})

        # find static lib: absolute path in -L then default
        find_library(_LIB_DIR NAMES ${_LIB} PATHS ${ADIOS_LIBRARY_DIRS})

        # found?
        if(_LIB_DIR)
            message(STATUS "Found ${_LIB} in ${_LIB_DIR}")
            list(APPEND ADIOS_LIBRARIES "${_LIB_DIR}")
        else(_LIB_DIR)
            set(ADIOS_FOUND FALSE)
            message(STATUS "ADIOS: Could NOT find library '${_LIB}'")
        endif(_LIB_DIR)

        # clean cached var
        unset(_LIB_DIR CACHE)
        unset(_LIB_DIR)
    endforeach()
    string(REGEX MATCHALL "^-l([A-Za-z_0-9\\.-]+)" _ADIOS_LIBS "${ADIOS_LINKFLAGS}")
    foreach(_LIB ${_ADIOS_LIBS})
        string(REPLACE "-l" "" _LIB ${_LIB})

        # find static lib: absolute path in -L then default
        find_library(_LIB_DIR NAMES ${_LIB} PATHS ${ADIOS_LIBRARY_DIRS} CMAKE_FIND_ROOT_PATH_BOTH)

        # found?
        if(_LIB_DIR)
            message(STATUS "Found ${_LIB} in ${_LIB_DIR}")
            list(APPEND ADIOS_LIBRARIES "${_LIB_DIR}")
        else(_LIB_DIR)
            set(ADIOS_FOUND FALSE)
            message(STATUS "ADIOS: Could NOT find library '${_LIB}'")
        endif(_LIB_DIR)

        # clean cached var
        unset(_LIB_DIR CACHE)
        unset(_LIB_DIR)
    endforeach()

    # add the version string
    execute_process(COMMAND ${ADIOS_CONFIG} -v
                    OUTPUT_VARIABLE ADIOS_VERSION
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

endif(ADIOS_FOUND)

# unset checked variables if not found
if(NOT ADIOS_FOUND)
    unset(ADIOS_INCLUDE_DIRS)
    unset(ADIOS_LIBRARIES)
endif(NOT ADIOS_FOUND)


# restore CMAKE_FIND_LIBRARY_SUFFIXES if manipulated by this module ###########
if(ADIOS_USE_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()


###############################################################################
# FindPackage Options
###############################################################################

# handles the REQUIRED, QUIET and version-related arguments for find_package
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ADIOS
    REQUIRED_VARS ADIOS_LIBRARIES ADIOS_INCLUDE_DIRS
    VERSION_VAR ADIOS_VERSION
)
