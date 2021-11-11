################################################################################
# ProjUtilities.cmake - part of CMake configuration of PROJ library
#
# Based on BoostUtilities.cmake from CMake configuration for Boost
################################################################################
# Copyright (C) 2007 Douglas Gregor <doug.gregor@gmail.com>
# Copyright (C) 2007 Troy Straszheim
# Copyright (C) 2010 Mateusz Loskot <mateusz@loskot.net>
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at
#   https://www.boost.org/LICENSE_1_0.txt
################################################################################
# Macros in this module:
#
#   print_variable
#   proj_target_output_name
#   configure_proj_pc
#
################################################################################

#
#  pretty-prints the value of a variable so that the
#  equals signs align
#

function(print_variable NAME)
  string(LENGTH "${NAME}" varlen)
  math(EXPR padding_len 30-${varlen})
  if(${padding_len} GREATER 0)
    string(SUBSTRING "                            "
      0 ${padding_len} varpadding)
  endif()
  message(STATUS "${NAME}${varpadding} = ${${NAME}}")
endfunction()

#
# Generates output name for given target depending on platform and version.
# For instance, on Windows, dynamic link libraries get ABI version suffix
# proj_X_Y.dll.
#

function(proj_target_output_name TARGET_NAME OUTPUT_NAME)
  if(NOT DEFINED TARGET_NAME)
    message(SEND_ERROR "Error, the variable TARGET_NAME is not defined!")
  endif()

  if(NOT DEFINED ${PROJECT_NAME}_VERSION)
    message(SEND_ERROR
      "Error, the variable ${${PROJECT_NAME}_VERSION} is not defined!")
  endif()

  # On Windows, ABI version is specified using binary file name suffix.
  # On Unix, suffix is empty and SOVERSION is used instead.
  if(WIN32)
    string(LENGTH "${${PROJECT_NAME}_ABI_VERSION}" abilen)
    if(abilen GREATER 0)
      set(SUFFIX "_${${PROJECT_NAME}_ABI_VERSION}")
    endif()
  endif()

  set(${OUTPUT_NAME} ${TARGET_NAME}${SUFFIX} PARENT_SCOPE)
endfunction()

#
# Configure a pkg-config file proj.pc
# See also ProjInstallPath.cmake
#

function(configure_proj_pc)
  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(exec_prefix "$\{prefix\}")
  set(libdir "$\{exec_prefix\}/${PROJ_LIB_SUBDIR}")
  set(includedir "$\{prefix\}/${PROJ_INCLUDE_SUBDIR}")
  set(datarootdir "$\{prefix\}/${CMAKE_INSTALL_DATAROOTDIR}")
  set(datadir "$\{datarootdir\}")
  set(PACKAGE "proj")
  set(VERSION ${PROJ_VERSION})
  set(SQLITE3_LIBS -lsqlite3)
  if(TIFF_ENABLED)
    set(TIFF_LIBS -ltiff)
  endif()
  if(CURL_ENABLED)
    set(CURL_LIBS -lcurl)
  endif()

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/proj.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/proj.pc
    @ONLY)
endfunction()
