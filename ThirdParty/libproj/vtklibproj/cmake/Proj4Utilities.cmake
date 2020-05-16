################################################################################
# SociUtilities.cmake - part of CMake configuration of Proj4 library
#
# Based on BoostUtilities.cmake from CMake configuration for Boost
################################################################################
# Copyright (C) 2007 Douglas Gregor <doug.gregor@gmail.com>
# Copyright (C) 2007 Troy Straszheim
# Copyright (C) 2010 Mateusz Loskot <mateusz@loskot.net> 
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at
#   http://www.boost.org/LICENSE_1_0.txt
################################################################################
# Macros in this module:
#
#   list_contains: Determine whether a string value is in a list.
#
#   car: Return the first element in a list
#
#   cdr: Return all but the first element in a list
#
#   parse_arguments: Parse keyword arguments for use in other macros.
#
#   proj_report_directory_property
#
#   proj_target_output_name: 
#
################################################################################

#
# A big shout out to the cmake gurus @ compiz
#

function (colormsg)
  return () # XXX(kitware): Hide configure noise.
  string (ASCII 27 _escape)
  set(WHITE "29")
  set(GRAY "30")
  set(RED "31")
  set(GREEN "32")
  set(YELLOW "33")
  set(BLUE "34")
  set(MAG "35")
  set(CYAN "36")

  foreach (color WHITE GRAY RED GREEN YELLOW BLUE MAG CYAN)
    set(HI${color} "1\;${${color}}")
    set(LO${color} "2\;${${color}}")
    set(_${color}_ "4\;${${color}}")
    set(_HI${color}_ "1\;4\;${${color}}")
    set(_LO${color}_ "2\;4\;${${color}}")
  endforeach()

  set(str "")
  set(coloron FALSE)
  foreach(arg ${ARGV})
    if (NOT ${${arg}} STREQUAL "")
      if (CMAKE_COLOR_MAKEFILE)
        set(str "${str}${_escape}[${${arg}}m")
        set(coloron TRUE)
      endif()
    else()
      set(str "${str}${arg}")
      if (coloron)
        set(str "${str}${_escape}[0m")
        set(coloron FALSE)
      endif()
      set(str "${str} ")
    endif()
  endforeach()
  message(STATUS ${str})
endfunction()

# colormsg("Colors:"  
#   WHITE "white" GRAY "gray" GREEN "green" 
#   RED "red" YELLOW "yellow" BLUE "blue" MAG "mag" CYAN "cyan" 
#   _WHITE_ "white" _GRAY_ "gray" _GREEN_ "green" 
#   _RED_ "red" _YELLOW_ "yellow" _BLUE_ "blue" _MAG_ "mag" _CYAN_ "cyan" 
#   _HIWHITE_ "white" _HIGRAY_ "gray" _HIGREEN_ "green" 
#   _HIRED_ "red" _HIYELLOW_ "yellow" _HIBLUE_ "blue" _HIMAG_ "mag" _HICYAN_ "cyan" 
#   HIWHITE "white" HIGRAY "gray" HIGREEN "green" 
#   HIRED "red" HIYELLOW "yellow" HIBLUE "blue" HIMAG "mag" HICYAN "cyan" 
#   "right?")

#
#  pretty-prints the value of a variable so that the 
#  equals signs align
#

function(boost_report_value NAME)
  return () # XXX(kitware): Hide configure noise.
  string(LENGTH "${NAME}" varlen)
  # LOG
  #message(STATUS "boost_report_value: NAME=${NAME} (${varlen})")
  #message(STATUS "boost_report_value: \${NAME}=${${NAME}}")
  math(EXPR padding_len 40-${varlen})
  string(SUBSTRING "                                      " 
    0 ${padding_len} varpadding)
  colormsg("${NAME}${varpadding} = ${${NAME}}")
endfunction()

function(trace NAME)
  if(BOOST_CMAKE_TRACE)
    string(LENGTH "${NAME}" varlen)
    math(EXPR padding_len 40-${varlen})
    string(SUBSTRING "........................................"
      0 ${padding_len} varpadding)
    message("${NAME} ${varpadding} ${${NAME}}")
  endif()
endfunction()  

#
#  pretty-prints the value of a variable so that the 
#  equals signs align
#
function(boost_report_pretty PRETTYNAME VARNAME)
  string(LENGTH "${PRETTYNAME}" varlen)
  math(EXPR padding_len 30-${varlen})
  string(SUBSTRING "                                      " 
    0 ${padding_len} varpadding)
  message(STATUS "${PRETTYNAME}${varpadding} = ${${VARNAME}}")
endfunction()

#
#  assert that ARG is actually a library target
#

macro(dependency_check ARG)
  trace(ARG)
  if (NOT "${ARG}" STREQUAL "")
    get_target_property(deptype ${ARG} TYPE)
    if(NOT deptype MATCHES ".*_LIBRARY$")
      set(DEPENDENCY_OKAY FALSE)
      list(APPEND DEPENDENCY_FAILURES ${ARG})
    endif()
  endif()
endmacro()



#
# Pretty-print of given property of current directory.
#
macro(proj_report_directory_property PROPNAME)
  get_directory_property(${PROPNAME} ${PROPNAME})
  boost_report_value(${PROPNAME})
endmacro()

#
# Scans the current directory and returns a list of subdirectories.
# Author: Robert Fleming
# Source: http://www.cmake.org/pipermail/cmake/2008-February/020114.html
#
# Third parameter is 1 if you want relative paths returned.
# Usage: list_subdirectories(the_list_is_returned_here /path/to/project TRUE)
#

macro(list_subdirectories retval curdir return_relative)
  file(GLOB sub-dir RELATIVE ${curdir} *)
  set(list_of_dirs "")
  foreach(dir ${sub-dir})
    if(IS_DIRECTORY ${curdir}/${dir})
      if (${return_relative})
        set(list_of_dirs ${list_of_dirs} ${dir})
      else()
        set(list_of_dirs ${list_of_dirs} ${curdir}/${dir})
      endif()
    endif()
  endforeach()
  set(${retval} ${list_of_dirs})
endmacro()

#
# Generates output name for given target depending on platform and version.
# For instance, on Windows, libraries get ABI version suffix soci_coreXY.{dll|lib}.
#

function(proj_target_output_name TARGET_NAME OUTPUT_NAME)
  if(NOT DEFINED TARGET_NAME)
    message(SEND_ERROR "Error, the variable TARGET_NAME is not defined!")
  endif()

  if(NOT DEFINED ${PROJECT_INTERN_NAME}_VERSION)
    message(SEND_ERROR "Error, the variable ${${PROJECT_INTERN_NAME}_VERSION} is not defined!")
  endif()

  # On Windows, ABI version is specified using binary file name suffix.
  # On Unix, suffix is empty and SOVERSION is used instead.
  if (WIN32)
    string(LENGTH "${${PROJECT_INTERN_NAME}_ABI_VERSION}" abilen)
    if(abilen GREATER 0)
      set(SUFFIX "_${${PROJECT_INTERN_NAME}_ABI_VERSION}")
    endif()
  endif()

  set(${OUTPUT_NAME} ${TARGET_NAME}${SUFFIX} PARENT_SCOPE)
endfunction()


#
# conversion from lla name to lla convert name ( without lla extension)
#

function(proj_lla_output_name LLA_INPUT_NAME LLA_OUTPUT_NAME  )
    get_filename_component(filename ${LLA_INPUT_NAME} NAME_WE)
    get_filename_component(pathname ${LLA_INPUT_NAME} PATH)
    set(${LLA_OUTPUT_NAME} ${pathname}/${filename} PARENT_SCOPE)
        set(${LLA_OUTPUT_NAME} ${pathname}/${filename} PARENT_SCOPE)
endfunction()

function(proj_lla_target_name LLA_INPUT_NAME  LLA_TARGET )
    get_filename_component(filename ${LLA_INPUT_NAME} NAME_WE)
    set(${LLA_TARGET} ${filename} PARENT_SCOPE)
endfunction()

#
# in place conversion of lla file to gsb 
#

function(proj_convert_grid_lla2gsb GRID_DIRECTORY) 
    set(NAD2BIN_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(NAD2BIN_PATH ${NAD2BIN_DIR}/nad2bin${CMAKE_EXECUTABLE_SUFFIX})
    file(TO_NATIVE_PATH ${NAD2BIN_PATH} NAD2BIN_EXE)
    file(GLOB LLA_FILES  ${${GRID_DIRECTORY}}/*.lla)
    foreach(LLA ${LLA_FILES} )
        proj_lla_output_name(${LLA} DEST_FILE)
        file(TO_NATIVE_PATH ${DEST_FILE} DEST)
        proj_lla_target_name(${LLA} LLA_TARGET)
        if(NOT EXISTS ${DEST})
            add_custom_target( ${LLA_TARGET} ALL
               COMMAND ${NAD2BIN_EXE} ${DEST} "<" ${LLA}
               DEPENDS nad2bin )
        endif(NOT EXISTS ${DEST})
    endforeach(LLA)
endfunction()

#
# add lla output list to an existing file list
#

function(proj_append_lla_output_file LLA_INPUT_FILE  FILE_LIST)
     set(LIST_OUT ${${FILE_LIST}})
     foreach(LLA ${${LLA_INPUT_FILE}} )
        proj_lla_output_name(${LLA} DEST_FILE)
        file(TO_NATIVE_PATH ${DEST_FILE} DEST)
        set(LIST_OUT ${LIST_OUT} ${DEST_FILE} )
    endforeach(LLA ${LLA_INPUT_FILE})
    set(${FILE_LIST} ${LIST_OUT} PARENT_SCOPE)
endfunction()

