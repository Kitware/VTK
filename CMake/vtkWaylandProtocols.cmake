# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause

#[==[.md

# Wayland Protocols integration in CMake

Find Waylands protocols, Wayland client and generate the xdg-shell protocol files using `wayland-scanner`.

## IMPORTED Targets

This module imports the following targets:

- `WAYLAND_CLIENT_LIB` : Defined path where the system has Wayland client library.
- `WAYLAND_EGL_LIB` : Defined path where the system has Wayland EGL library.

## Result Variables

This module defines the following variables:

- `WAYLAND_PROTOCOLS_DIR` : Path where the Wayland protocols are installed on the system.
- `XDG_SHELL_C` : Path to the generated xdg-shell protocol C file.
- `XDG_SHELL_H` : Path to the generated xdg-shell protocol header file.

#]==]

find_library(WAYLAND_CLIENT_LIB wayland-client REQUIRED)
find_library(WAYLAND_EGL_LIB wayland-egl REQUIRED)

# Find the Wayland protocols with pkg to avoid hardcoding the path
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(WAYLAND_PROTOCOLS QUIET wayland-protocols)
  if(WAYLAND_PROTOCOLS_FOUND)
    execute_process(
      COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=pkgdatadir wayland-protocols
      OUTPUT_VARIABLE WAYLAND_PROTOCOLS_PKGDATADIR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endif()

# Our Wayland implementation requires the xdg-shell protocol
find_path(WAYLAND_PROTOCOLS_DIR
  NAMES stable/xdg-shell/xdg-shell.xml
  HINTS "${WAYLAND_PROTOCOLS_PKGDATADIR}"
  REQUIRED)

if (NOT WAYLAND_PROTOCOLS_DIR)
  message(FATAL_ERROR "xdg-shell protocol not found. Install the wayland-protocols.")
endif()

# Generate the xdg-shell protocol C and header files
set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
set(XDG_SHELL_C "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocols.c")
set(XDG_SHELL_H "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocols.h")

add_custom_command(
  OUTPUT ${XDG_SHELL_C} ${XDG_SHELL_H}
  COMMAND wayland-scanner client-header ${XDG_SHELL_XML} ${XDG_SHELL_H}
  COMMAND wayland-scanner private-code ${XDG_SHELL_XML} ${XDG_SHELL_C}
  DEPENDS ${XDG_SHELL_XML}
  COMMENT "Generating xdg-shell protocol files")
