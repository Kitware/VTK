#
# Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#


# Adjust the library search path based on the bit-ness of the build.
# (i.e. 64: bin64, lib64; 32: bin, lib).
# Note that on Mac, the OptiX library is a universal binary, so we
# only need to look in lib and not lib64 for 64 bit builds.
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT APPLE)
  set(bit_dest "64")
else()
  set(bit_dest "")
endif()

set(OptiX_INSTALL_DIR "" CACHE PATH "Path to OptiX installed location.")

macro(OPTIX_find_api_library name version)
  find_library(${name}_LIBRARY
    NAMES ${name}.${version} ${name}
    PATHS "${OptiX_INSTALL_DIR}/lib${bit_dest}"
    NO_DEFAULT_PATH
    )
  find_library(${name}_LIBRARY
    NAMES ${name}.${version} ${name}
    )
  if(WIN32)
    find_file(${name}_DLL
      NAMES ${name}.${version}.dll
      PATHS "${OptiX_INSTALL_DIR}/bin${bit_dest}"
      NO_DEFAULT_PATH
      )
    find_file(${name}_DLL
      NAMES ${name}.${version}.dll
      )
  endif()
endmacro()

# OptiX SDKs before 5.1.0 named the library optix.1.lib
# Linux handles this via symlinks and doesn't need changes to the library name.
set(OptiX_version "1")

# The OptiX library is named with the major and minor digits since OptiX 5.1.0.
# Dynamically find the matching library name by parsing the OptiX_INSTALL_DIR.
# This only works if the installation retained the original folder format "OptiX SDK major.minor.micro".
# We want the concatenated major and minor numbers if the version is greater or equal to 5.1.0.
if(WIN32)
  if(OptiX_INSTALL_DIR)
    string(REGEX REPLACE " |-" ";" OptiX_install_dir_list ${OptiX_INSTALL_DIR})
    list(LENGTH OptiX_install_dir_list OptiX_install_dir_list_length)
    if(${OptiX_install_dir_list_length} GREATER 0)
      # Get the last list element, something like "5.1.0".
      list(GET OptiX_install_dir_list -1 OptiX_version_string)
      # Component-wise integer version number comparison (version format is major[.minor[.patch[.tweak]]]).
      # Starting with OptiX 6.0.0, the full version number is used to avoid the Windows DLL hell.
      if(${OptiX_version_string} VERSION_GREATER_EQUAL "6.0.0")
        set(OptiX_version ${OptiX_version_string})
      elseif(${OptiX_version_string} VERSION_GREATER_EQUAL "5.1.0")
        set(OptiX_version "")
        string(REPLACE "." ";" OptiX_major_minor_micro_list ${OptiX_version_string})
        foreach(index RANGE 0 1)
          list(GET OptiX_major_minor_micro_list ${index} number)
          string(APPEND OptiX_version ${number})
        endforeach()
      endif()
    endif()
  endif()
endif(WIN32)



OPTIX_find_api_library(optix ${OptiX_version})
OPTIX_find_api_library(optixu ${OptiX_version})
OPTIX_find_api_library(optix_prime ${OptiX_version})

# Include
find_path(OptiX_INCLUDE
  NAMES optix.h
  PATHS "${OptiX_INSTALL_DIR}/include"
  NO_DEFAULT_PATH
  )
find_path(OptiX_INCLUDE
  NAMES optix.h
  )

# Check to make sure we found what we were looking for
function(OptiX_report_error error_message required)
  if(OptiX_FIND_REQUIRED AND required)
    message(FATAL_ERROR "${error_message}")
  else()
    if(NOT OptiX_FIND_QUIETLY)
      message(STATUS "${error_message}")
    endif(NOT OptiX_FIND_QUIETLY)
  endif()
endfunction()

if(NOT optix_LIBRARY)
  OptiX_report_error("OptiX library not found. Please set OptiX_INSTALL_DIR to locate it automatically." TRUE)
endif()
if(NOT OptiX_INCLUDE)
  OptiX_report_error("OptiX headers (optix.h and friends) not found. Please set OptiX_INSTALL_DIR to locate them automatically." TRUE)
endif()
if(NOT optix_prime_LIBRARY)
  OptiX_report_error("OptiX Prime library not found. Please set OptiX_INSTALL_DIR to locate it automatically." FALSE)
endif()

# Macro for setting up dummy targets
function(OptiX_add_imported_library name lib_location dll_lib dependent_libs)
  set(CMAKE_IMPORT_FILE_VERSION 1)

  # Create imported target
  add_library(${name} SHARED IMPORTED)

  # Import target "optix" for configuration "Debug"
  if(WIN32)
    set_target_properties(${name} PROPERTIES
      IMPORTED_IMPLIB "${lib_location}"
      #IMPORTED_LINK_INTERFACE_LIBRARIES "glu32;opengl32"
      IMPORTED_LOCATION "${dll_lib}"
      IMPORTED_LINK_INTERFACE_LIBRARIES "${dependent_libs}"
      )
  elseif(UNIX)
    set_target_properties(${name} PROPERTIES
      #IMPORTED_LINK_INTERFACE_LIBRARIES "glu32;opengl32"
      IMPORTED_LOCATION "${lib_location}"
      # We don't have versioned filenames for now, and it may not even matter.
      #IMPORTED_SONAME "${optix_soname}"
      IMPORTED_LINK_INTERFACE_LIBRARIES "${dependent_libs}"
      )
  else()
    # Unknown system, but at least try and provide the minimum required
    # information.
    set_target_properties(${name} PROPERTIES
      IMPORTED_LOCATION "${lib_location}"
      IMPORTED_LINK_INTERFACE_LIBRARIES "${dependent_libs}"
      )
  endif()

  # Commands beyond this point should not need to know the version.
  set(CMAKE_IMPORT_FILE_VERSION)
endfunction()

# Sets up a dummy target
OptiX_add_imported_library(optix "${optix_LIBRARY}" "${optix_DLL}" "${OPENGL_LIBRARIES}")
OptiX_add_imported_library(optixu "${optixu_LIBRARY}" "${optixu_DLL}" "")
OptiX_add_imported_library(optix_prime "${optix_prime_LIBRARY}" "${optix_prime_DLL}" "")

macro(OptiX_check_same_path libA libB)
  if(_optix_path_to_${libA})
    if(NOT _optix_path_to_${libA} STREQUAL _optix_path_to_${libB})
      # ${libA} and ${libB} are in different paths.  Make sure there isn't a ${libA} next
      # to the ${libB}.
      get_filename_component(_optix_name_of_${libA} "${${libA}_LIBRARY}" NAME)
      if(EXISTS "${_optix_path_to_${libB}}/${_optix_name_of_${libA}}")
        message(WARNING " ${libA} library found next to ${libB} library that is not being used.  Due to the way we are using rpath, the copy of ${libA} next to ${libB} will be used during loading instead of the one you intended.  Consider putting the libraries in the same directory or moving ${_optix_path_to_${libB}}/${_optix_name_of_${libA} out of the way.")
      endif()
    endif()
    set( _${libA}_rpath "-Wl,-rpath,${_optix_path_to_${libA}}" )
  endif()
endmacro()

# Since liboptix.1.dylib is built with an install name of @rpath, we need to
# compile our samples with the rpath set to where optix exists.
if(APPLE)
  get_filename_component(_optix_path_to_optix "${optix_LIBRARY}" PATH)
  if(_optix_path_to_optix)
    set( _optix_rpath "-Wl,-rpath,${_optix_path_to_optix}" )
  endif()
  get_filename_component(_optix_path_to_optixu "${optixu_LIBRARY}" PATH)
  OptiX_check_same_path(optixu optix)
  get_filename_component(_optix_path_to_optix_prime "${optix_prime_LIBRARY}" PATH)
  OptiX_check_same_path(optix_prime optix)
  OptiX_check_same_path(optix_prime optixu)

  set( optix_rpath ${_optix_rpath} ${_optixu_rpath} ${_optix_prime_rpath} )
  list(REMOVE_DUPLICATES optix_rpath)
endif()
