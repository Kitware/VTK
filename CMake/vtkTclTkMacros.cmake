# ----------------------------------------------------------------------------
# VTK_GET_TCL_TK_VERSION
# Return the major/minor version of the Tcl/Tk library used by VTK.
#
# in: tcl_tk_major_version: name of the var the major version is written to
#     tcl_tk_minor_version: name of the var the minor version is written to
#
# ex: VTK_GET_TCL_TK_VERSION ("TCL_TK_MAJOR_VERSION" "TCL_TK_MINOR_VERSION")
#     SET (TCL_TK_VERSION "${TCL_TK_MAJOR_VERSION}.${TCL_TK_MINOR_VERSION}")

MACRO (VTK_GET_TCL_TK_VERSION tcl_tk_major_version tcl_tk_minor_version)

  # Try to find the current Tcl/Tk version by matching TK_INTERNAL_PATH
  # or TCL_LIBRARY against some version numbers

  SET (${tcl_tk_major_version} "")
  SET (${tcl_tk_minor_version} "")
  IF (TK_INTERNAL_PATH)
    SET (try_tk_internal_path ${TK_INTERNAL_PATH})
  ELSE (TK_INTERNAL_PATH)
    SET (try_tk_internal_path ${VTK_TK_INTERNAL_DIR})
  ENDIF (TK_INTERNAL_PATH)
  FOREACH (tcl_tk_minor_version_try "2" "3" "4")
    IF ("${try_tk_internal_path}" MATCHES "tk8\\.?${tcl_tk_minor_version_try}")
      SET (${tcl_tk_major_version} "8")
      SET (${tcl_tk_minor_version} ${tcl_tk_minor_version_try})
    ENDIF ("${try_tk_internal_path}" MATCHES "tk8\\.?${tcl_tk_minor_version_try}")
    IF ("${TCL_LIBRARY}" MATCHES "tcl8\\.?${tcl_tk_minor_version_try}")
      SET (${tcl_tk_major_version} "8")
      SET (${tcl_tk_minor_version} ${tcl_tk_minor_version_try})
    ENDIF ("${TCL_LIBRARY}" MATCHES "tcl8\\.?${tcl_tk_minor_version_try}")
    IF ("${TCL_INCLUDE_PATH}" MATCHES "tcl8\\.?${tcl_tk_minor_version_try}")
      SET (${tcl_tk_major_version} "8")
      SET (${tcl_tk_minor_version} ${tcl_tk_minor_version_try})
    ENDIF ("${TCL_INCLUDE_PATH}" MATCHES "tcl8\\.?${tcl_tk_minor_version_try}")
    # Mac
    IF ("${TCL_INCLUDE_PATH}" MATCHES "Tcl.*8\\.${tcl_tk_minor_version_try}")
      SET (${tcl_tk_major_version} "8")
      SET (${tcl_tk_minor_version} ${tcl_tk_minor_version_try})
    ENDIF ("${TCL_INCLUDE_PATH}" MATCHES "Tcl.*8\\.${tcl_tk_minor_version_try}")
  ENDFOREACH (tcl_tk_minor_version_try)

  FOREACH(dir ${TCL_INCLUDE_PATH})
    IF(EXISTS "${dir}/tcl.h")
      FILE(READ "${dir}/tcl.h" tcl_include_file)
      STRING(REGEX REPLACE
        ".*#define TCL_VERSION[ \t]*\"([0-9][0-9]*\\.[0-9][0-9]*)\".*" "\\1"
        tcl_include_file "${tcl_include_file}")
      IF(${tcl_include_file} MATCHES "^[0-9]*\\.[0-9]*$")
        STRING(REGEX REPLACE "^([0-9]*)\\.([0-9]*)$" "\\1" ${tcl_tk_major_version}
          "${tcl_include_file}")
        STRING(REGEX REPLACE "^([0-9]*)\\.([0-9]*)$" "\\2" ${tcl_tk_minor_version}
          "${tcl_include_file}")
      ENDIF(${tcl_include_file} MATCHES "^[0-9]*\\.[0-9]*$")
    ENDIF(EXISTS "${dir}/tcl.h")
  ENDFOREACH(dir)

ENDMACRO (VTK_GET_TCL_TK_VERSION)

# ----------------------------------------------------------------------------
# VTK_GET_TCL_SUPPORT_FILES, VTK_GET_TK_SUPPORT_FILES
# Get a list of Tcl/Tk support files for a given Tcl/Tk repository.
# Tcl/Tk support files are additional files that are mandatory for Tcl/Tk
# to work properly. Linking against Tcl/Tk shared/static library is just
# not enough, Tcl/Tk needs to access those files at run-time.
# A typical Tcl/Tk installation will store support files in sub-directories 
# inside the Tcl/Tk lib directory, organized by version number. 
# Example:
#    c:/tcl/lib/tcl8.4
#    c:/tcl/lib/tcl8.3
#    c:/tcl/lib/tk8.4
#    c:/tcl/lib/tk8.3
# A typical source repository is organized differently:
#    c:/tcl8.4.5/library
#    c:/tk8.4.5/library
# Given the path to the Tcl support lib dir, VTK_GET_TCL_SUPPORT_FILES will
# return the corresponding list of support files.
# Given the path to the Tk support lib dir, VTK_GET_TK_SUPPORT_FILES will
# return the corresponding list of support files.
#
# in: support_lib_dir: path to the Tcl (or Tk) support lib dir
#     list:            name of the var the list is written to

MACRO (VTK_GET_TCL_SUPPORT_FILES tcl_support_lib_dir list)

  # Tcl support files (*.tcl + encoding + tclIndex, etc.)

  FILE (GLOB TCL_SUPPORT_FILES_TCL "${tcl_support_lib_dir}/*.tcl")
  FILE (GLOB TCL_SUPPORT_FILES_ENC "${tcl_support_lib_dir}/encoding/*.enc")
  SET (${list}
    "${tcl_support_lib_dir}/tclIndex" 
    ${TCL_SUPPORT_FILES_TCL} 
    ${TCL_SUPPORT_FILES_ENC})

ENDMACRO (VTK_GET_TCL_SUPPORT_FILES)

MACRO (VTK_GET_TK_SUPPORT_FILES tk_support_lib_dir list)

  # Tk support files (*.tcl + tclIndex, etc.)

  FILE (GLOB TK_SUPPORT_FILES_TCL "${tk_support_lib_dir}/*.tcl")
  SET (${list}
    "${tk_support_lib_dir}/tclIndex" 
    ${TK_SUPPORT_FILES_TCL})

ENDMACRO (VTK_GET_TK_SUPPORT_FILES)

# ----------------------------------------------------------------------------
# VTK_COPY_TCL_TK_SUPPORT_FILES
# Copy (or install) Tcl/Tk support files to a specific location.
# See VTK_GET_TCL_SUPPORT_FILES for more info about support files.
# Given the paths to the Tcl and Tk support lib dirs, this macro will 
# copy (or install) the appropriate support files to destination dirs, 
# recreating the subdirs.
# This macro takes an optional last parameter, if set to INSTALL the 
# files will be scheduled for installation (using CMake's INSTALL)
# instead of copied.
#
# in: tcl_support_lib_dir:  path to the Tcl support lib dir
#     tcl_support_lib_dest: destination dir for the Tcl support lib files
#     tk_support_lib_dir:   path to the Tk support lib dir
#     tk_support_lib_dest:  destination dir for the Tk support lib files
#     INSTALL:              optional parameter (install files instead of copy)
#
# ex: VTK_COPY_TCL_TK_SUPPORT_FILES (
#       "c:/tcl/lib/tcl8.4" "d:/vtk-bin/TclTk/lib/tcl8.4"
#       "c:/tcl/lib/tk8.4" "d:/vtk-bin/TclTk/lib/tk8.4")
#     this will copy support files from:
#       c:/tcl/lib/tcl8.4
#       c:/tcl/lib/tk8.4
#     to:
#       d:/vtk-bin/TclTk/lib/tcl8.4
#       d:/vtk-bin/TclTk/lib/tk8.4

MACRO (VTK_COPY_TCL_TK_SUPPORT_FILES tcl_support_lib_dir tcl_support_lib_dest tk_support_lib_dir tk_support_lib_dest)

  # Get the support files and copy them to dest dir
  # Check if EXISTS to work around CONFIGURE_FILE bug (if file does not
  # exist, it would create the subdirs anyway)

  VTK_GET_TCL_SUPPORT_FILES(${tcl_support_lib_dir} "TCL_SUPPORT_FILES")
  STRING(REGEX REPLACE "^/" "" tcl_support_lib_dest_cm24 "${tcl_support_lib_dest}")
  STRING(REGEX REPLACE "^/" "" tk_support_lib_dest_cm24 "${tk_support_lib_dest}")
  FOREACH (file ${TCL_SUPPORT_FILES})
    IF (EXISTS ${file})
      STRING (REGEX REPLACE "${tcl_support_lib_dir}/" "" filebase ${file})
      IF ("${ARGV4}" STREQUAL "INSTALL")
        GET_FILENAME_COMPONENT(dir ${filebase} PATH)
        INSTALL(FILES "${file}"
          DESTINATION "${tcl_support_lib_dest_cm24}/${dir}"
          COMPONENT RuntimeLibraries)
      ELSE ("${ARGV4}" STREQUAL "INSTALL")
        CONFIGURE_FILE (${file} "${tcl_support_lib_dest}/${filebase}" COPYONLY)
      ENDIF ("${ARGV4}" STREQUAL "INSTALL")
    ENDIF (EXISTS ${file})
  ENDFOREACH (file)

  VTK_GET_TK_SUPPORT_FILES(${tk_support_lib_dir} "TK_SUPPORT_FILES")
  FOREACH (file ${TK_SUPPORT_FILES})
    IF (EXISTS ${file})
      STRING (REGEX REPLACE "${tk_support_lib_dir}/" "" filebase ${file})
      IF ("${ARGV4}" STREQUAL "INSTALL")
        GET_FILENAME_COMPONENT(dir ${filebase} PATH)
        INSTALL(FILES "${file}"
          DESTINATION "${tk_support_lib_dest_cm24}/${dir}"
          COMPONENT RuntimeLibraries)
      ELSE ("${ARGV4}" STREQUAL "INSTALL")
        CONFIGURE_FILE (${file} "${tk_support_lib_dest}/${filebase}" COPYONLY)
      ENDIF ("${ARGV4}" STREQUAL "INSTALL")
    ENDIF (EXISTS ${file})
  ENDFOREACH (file)

ENDMACRO (VTK_COPY_TCL_TK_SUPPORT_FILES)

# ----------------------------------------------------------------------------
# VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR
# Front-end to VTK_COPY_TCL_TK_SUPPORT_FILES, this macro will 
# copy (or install) the appropriate Tcl/Tk support files to a directory.
# The Tcl/Tk version is retrieved automatically and used to create
# the subdirectories (see example below)
# This macro takes an optional last parameter, if set to INSTALL the 
# files will be scheduled for installation (using CMake's INSTALL)
# instead of copied.
#
# in: tcl_support_lib_dir: path to the Tcl support lib dir
#     tk_support_lib_dir:  path to the Tk support lib dir
#     target_dir:          target directory
#     INSTALL:             optional parameter (install files instead of copy)
#
# ex: VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR (
#        "c:/tcl/lib/tcl8.4" "c:/tcl/lib/tk8.4" "d:/vtk-bin/lib")
#     if this project is configured to use TclTk 8.4, this will copy support 
#     files from:
#       c:/tcl/lib/tcl8.4
#       c:/tcl/lib/tk8.4
#     to:
#       d:/vtk-bin/lib/tcl8.4
#       d:/vtk-bin/lib/tk8.4

MACRO (VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR tcl_support_lib_dir tk_support_lib_dir target_dir)
  VTK_GET_TCL_TK_VERSION ("TCL_TK_MAJOR_VERSION" "TCL_TK_MINOR_VERSION")
  IF (TCL_TK_MAJOR_VERSION AND TCL_TK_MINOR_VERSION)
    SET (TCL_TK_VERSION "${TCL_TK_MAJOR_VERSION}.${TCL_TK_MINOR_VERSION}")
    VTK_COPY_TCL_TK_SUPPORT_FILES (
      "${tcl_support_lib_dir}"
      "${target_dir}/tcl${TCL_TK_VERSION}"
      "${tk_support_lib_dir}"
      "${target_dir}/tk${TCL_TK_VERSION}"
      "${ARGV3}"
      )
  ENDIF (TCL_TK_MAJOR_VERSION AND TCL_TK_MINOR_VERSION)

ENDMACRO (VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR)

# ----------------------------------------------------------------------------
# VTK_COPY_TCL_TK_SUPPORT_FILES_TO_BUILD_DIR
# Front-end to VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR, this macro will copy the
# appropriate Tcl/Tk support files to a project build directory.
# The support files will be copied simultaneously to all configuration 
# sub-directories (Release, RelInfo, Debug, etc.) if needed.
# The Tcl/Tk version is retrieved automatically and used to create
# the subdirectories (see example below)
#
# in: tcl_support_lib_dir: path to the Tcl support lib dir
#     tk_support_lib_dir:  path to the Tk support lib dir
#     build_dir:           project build dir
#     dir:                 relative subdir inside the build dir, into which the
#                          support files will be copied.
#
# ex: VTK_COPY_TCL_TK_SUPPORT_FILES_TO_BUILD_DIR (
#        "c:/tcl/lib/tcl8.4" "c:/tcl/lib/tk8.4" "d:/vtk-bin" "TclTk/lib")
#     if this project is configured to use TclTk 8.4, this will copy support 
#     files from:
#       c:/tcl/lib/tcl8.4
#       c:/tcl/lib/tk8.4
#     to:
#       d:/vtk-bin/TclTk/lib/tcl8.4
#       d:/vtk-bin/TclTk/lib/tk8.4
#     or (if configuration types are supported by the generator):
#       d:/vtk-bin/Release/TclTk/lib/tcl8.4
#       d:/vtk-bin/Release/TclTk/lib/tk8.4
#       d:/vtk-bin/Debug/TclTk/lib/tcl8.4
#       d:/vtk-bin/Debug/TclTk/lib/tk8.4
#       etc.

MACRO (VTK_COPY_TCL_TK_SUPPORT_FILES_TO_BUILD_DIR tcl_support_lib_dir tk_support_lib_dir build_dir dir)

  # For each configuration type (Debug, RelInfo, Release, etc.)
  # Copy the TclTk support files to the corresponding sub-directory inside
  # the build dir

  IF (CMAKE_CONFIGURATION_TYPES)
    SET (CONFIG_TYPES ${CMAKE_CONFIGURATION_TYPES})
  ELSE (CMAKE_CONFIGURATION_TYPES)
    SET (CONFIG_TYPES .)
  ENDIF (CMAKE_CONFIGURATION_TYPES)
  FOREACH (config ${CONFIG_TYPES})
    VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR (
      "${tcl_support_lib_dir}"
      "${tk_support_lib_dir}"
      "${build_dir}/${config}/${dir}"
      )
  ENDFOREACH (config)

ENDMACRO (VTK_COPY_TCL_TK_SUPPORT_FILES_TO_BUILD_DIR)
