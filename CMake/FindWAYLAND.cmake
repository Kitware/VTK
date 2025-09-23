# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill
# Lorensen SPDX-FileCopyrightText: Copyright 2010-2012 Kitware, Inc.
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:

FindWAYLAND
-----------
Find Wayland library and associated protocols.

Usage
^^^^^

Use this module by invoking

    find_package(Wayland [REQUIRED] [version] [QUIET])

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

 WAYLAND_FOUND - system has Wayland

 WAYLAND_INCLUDE_DIRS - the Wayland include directories

 WAYLAND_LIBRARIES - Wayland libraries to be linked

 WAYLAND_SCANNER_EXECUTABLE - wayland-scanner executable

The module also defines the following imported targets:

 WAYLAND::Client - imported target for Wayland client library

 WAYLAND::Cursor - imported target for Wayland cursor library

Functions
^^^^^^^^^

This module defines the following functions:

  wayland_generate_protocol(output_var xml_file client_header client_source)

    Generate Wayland protocol files from the given XML file. The generated
    files are added to the output_var list variable.

  wayland_generate_xdg_protocol(output_var)

    Generate the xdg-shell protocol files. The generated files are added to
    the output_var list variable.

#]=======================================================================]

# Use pkg-config to find wayland-client and wayland-cursor
find_package(PkgConfig)
pkg_check_modules(
  PKG_WAYLAND
  REQUIRED
  IMPORTED_TARGET
  wayland-client
  wayland-cursor
  wayland-protocols
)

set(WAYLAND_DEFINITIONS
    ${PKG_WAYLAND_CFLAGS}
)

# wayland-client
find_path(
  WAYLAND_CLIENT_INCLUDE_DIR
  NAMES wayland-client.h
  HINTS ${PKG_WAYLAND_INCLUDE_DIRS}
)
find_library(
  WAYLAND_CLIENT_LIBRARY
  NAMES wayland-client
  HINTS ${PKG_WAYLAND_LIBRARY_DIRS}
)

if(NOT
   TARGET
   WAYLAND::Client
)
  add_library(
    WAYLAND::Client
    UNKNOWN
    IMPORTED
  )
  set_target_properties(
    WAYLAND::Client
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
               "${WAYLAND_CLIENT_INCLUDE_DIR}"
               IMPORTED_LOCATION
               "${WAYLAND_CLIENT_LIBRARY}"
  )
endif()

# wayland-cursor
find_path(
  WAYLAND_CURSOR_INCLUDE_DIR
  NAMES wayland-cursor.h
  HINTS ${PKG_WAYLAND_INCLUDE_DIRS}
)
find_library(
  WAYLAND_CURSOR_LIBRARY
  NAMES wayland-cursor
  HINTS ${PKG_WAYLAND_LIBRARY_DIRS}
)

if(NOT
   TARGET
   WAYLAND::Cursor
)
  add_library(
    WAYLAND::Cursor
    UNKNOWN
    IMPORTED
  )
  set_target_properties(
    WAYLAND::Cursor
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
               "${WAYLAND_CURSOR_INCLUDE_DIR}"
               IMPORTED_LOCATION
               "${WAYLAND_CURSOR_LIBRARY}"
  )
endif()

set(WAYLAND_INCLUDE_DIRS
    ${WAYLAND_CLIENT_INCLUDE_DIR}
    ${WAYLAND_CURSOR_INCLUDE_DIR}
)

set(WAYLAND_LIBRARIES
    ${WAYLAND_CLIENT_LIBRARY}
    ${WAYLAND_CURSOR_LIBRARY}
)

list(
  REMOVE_DUPLICATES
  WAYLAND_INCLUDE_DIRS
)

mark_as_advanced(
  WAYLAND_CLIENT_INCLUDE_DIR
  WAYLAND_CLIENT_LIBRARY
  WAYLAND_CURSOR_INCLUDE_DIR
  WAYLAND_CURSOR_LIBRARY
)

find_program(
  WAYLAND_SCANNER_EXECUTABLE
  NAMES wayland-scanner
)
mark_as_advanced(WAYLAND_SCANNER_EXECUTABLE)

pkg_get_variable(
  WAYLAND_PROTOCOLS_DATADIR
  wayland-protocols
  pkgdatadir
)

function(
  wayland_generate_protocol
  output_var
  xml_file
  client_header
  client_source
)
  get_filename_component(
    in_file
    ${xml_file}
    ABSOLUTE
  )
  add_custom_command(
    OUTPUT ${client_header}
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${in_file}
            ${client_header}
    DEPENDS ${in_file}
            ${WAYLAND_SCANNER_EXECUTABLE}
    COMMENT "Generating ${client_header}"
  )
  add_custom_command(
    OUTPUT ${client_source}
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${in_file}
            ${client_source}
    DEPENDS ${in_file}
            ${WAYLAND_SCANNER_EXECUTABLE}
    COMMENT "Generating ${client_source}"
  )
  list(
    APPEND
    ${output_var}
    "${client_header}"
    "${client_source}"
  )
  set(${output_var}
      "${${output_var}}"
      PARENT_SCOPE
  )
endfunction()

function(
  wayland_generate_xdg_protocol
  output_var
)
  find_file(
    WAYLAND_XDG_XML
    NAMES xdg-shell.xml
    HINTS ${WAYLAND_INCLUDE_DIRS}
          ${WAYLAND_PROTOCOLS_DATADIR}/stable/xdg-shell
          ${WAYLAND_PROTOCOLS_DATADIR}/xdg-shell
  )
  wayland_generate_protocol(
    ${output_var}
    ${WAYLAND_XDG_XML}
    xdg-shell-protocol.h
    xdg-shell-protocol.c
  )

  find_file(
    WAYLAND_XDG_DECORATION_XML
    NAMES xdg-decoration-unstable-v1.xml
    HINTS ${WAYLAND_INCLUDE_DIRS}
          ${WAYLAND_PROTOCOLS_DATADIR}/unstable/xdg-decoration
          ${WAYLAND_PROTOCOLS_DATADIR}/xdg-decoration
  )
  wayland_generate_protocol(
    ${output_var}
    ${WAYLAND_XDG_DECORATION_XML}
    xdg-decoration-protocol.h
    xdg-decoration-protocol.c
  )

  set(${output_var}
      "${${output_var}}"
      PARENT_SCOPE
  )
endfunction()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  WAYLAND
  FOUND_VAR WAYLAND_FOUND
  REQUIRED_VARS
    WAYLAND_LIBRARIES
    WAYLAND_INCLUDE_DIRS
    WAYLAND_SCANNER_EXECUTABLE
)
