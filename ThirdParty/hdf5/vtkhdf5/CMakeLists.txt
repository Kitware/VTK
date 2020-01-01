if (FALSE) # XXX(kitware): cmake versions are handled by VTK.
cmake_minimum_required (VERSION 3.10)
endif ()
project (HDF5 C)

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# XXX(kitware): Hardcode settings.
set(DISABLE_PDB_FILES ON)

#-----------------------------------------------------------------------------
# Instructions for use : Normal Build
#
# For standard build of HDF5 libraries,tests and tools.
# Run cmake using the HDF5 source tree to generate a build tree.
# Enable/Disable options according to requirements and
# set CMAKE_INSTALL_PREFIX to the required install path.
# Make install can be used to install all components for system-wide use.
#
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    MESSAGE(FATAL_ERROR "\nERROR! ${PROJECT_NAME} DOES NOT SUPPORT IN SOURCE BUILDS!\n"
      "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      " == CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}\n"
      "NEXT STEPS:\n"
      "(1) Delete the CMakeCache.txt file and the CMakeFiles/ directory\n"
      "     under the source directory for ${PROJECT_NAME}, otherwise you\n"
      "     will not be able to configure ${PROJECT_NAME} correctly!\n"
      "      * For example, on linux machines do:\n"
      "        $ rm -r CMakeCache.txt CMakeFiles/\n"
      "(2) Create a different directory and configure ${PROJECT_NAME} in that directory.\n"
      "      * For example, on linux machines do:\n"
      "        $ mkdir MY_BUILD\n"
      "        $ cd MY_BUILD\n"
      "        $ cmake [OPTIONS] ..\n"
      )
endif ()

# CMake version 3.14 added option --ignore-eol to compare files
#     cmake -E compare_files --ignore-eol file1 file2
#if(CMAKE_VERSION VERSION_LESS "3.14.0" AND WIN32)
#    MESSAGE(FATAL_ERROR "Windows builds requires a minimum of CMake 3.14")
#endif()

#-----------------------------------------------------------------------------
# Instructions for use : Sub-Project Build
#
# To include HDF5 as a sub-project within another project.
# Set HDF5_EXTERNALLY_CONFIGURED to 1 in the parent project and
# supply values for the following variables...
#
# HDF5_EXPORTED_TARGETS :
#   Set this to the name of the targets variable which controls exports
#   If unset (because parent project does not support/use the
#   install (EXPORT target...) syntax), then targets are not configured
#   for export during install.
#
# HDF5_LIB_DEPENDENCIES :
#   If the build of HDF5 libs is being customized, then rules for the
#   dependencies of the HDF5 libs may be 'incomplete', add additional
#   dependencies to this variable so that external projects pick them up
#
#option (HDF5_EXTERNAL_LIB_PREFIX "Use prefix for custom library naming." "")
if (FALSE) # XXX(kitware): Hardcode settings.
set (HDF5_EXTERNAL_LIB_PREFIX "" CACHE STRING "Use prefix for custom library naming.")
mark_as_advanced (HDF5_EXTERNAL_LIB_PREFIX)
else ()
set(HDF5_EXTERNAL_LIB_PREFIX "")
endif ()
# HDF5_EXTERNAL_LIB_PREFIX :
#   If the parent project needs to install hdf libraries, but avoid
#   name conflicts with system versions, then a prefix may be added
#   to ensure that the correct versions configured are used.
#
# HDF5_INSTALL_BIN_DIR, HDF5_INSTALL_LIB_DIR, HDF5_INSTALL_INCLUDE_DIR, HDF5_INSTALL_DATA_DIR :
#   Customize the 'bin', 'lib', 'include', and 'share' installation directories.
#
# HDF5_INSTALL_NO_DEVELOPMENT :
#   Set to true to skip installation of headers and CMake package files.
#
# Consider this example from the ParaView project, it builds its own zlib
# library and tells HDF5 to add it as a dependency - this ensures that
# any project making use of this build of HDF5 will use the correct zlib
#
#   # Tell hdf5 that we are manually overriding certain settings
#   set (HDF5_EXTERNALLY_CONFIGURED 1)
#   # Avoid duplicating names of installed libraries
#   set (HDF5_EXTERNAL_LIB_PREFIX "vtk")
#   # Export configuration to this export variable
#   set (HDF5_EXPORTED_TARGETS "paraview-targets")
#
#   # Setup all necessary overrides for zlib so that HDF5 uses our
#   # internally compiled zlib rather than any other version
#   if (HDF5_ENABLE_Z_LIB_SUPPORT)
#     # We must tell the main HDF5 library that it depends on our zlib
#     set (HDF5_LIB_DEPENDENCIES vtkzlib)
#     # Override the zlib header file
#     if (VTK_USE_SYSTEM_ZLIB)
#       set (H5_ZLIB_HEADER "zlib.h")
#     else ()
#       set (H5_ZLIB_HEADER "vtk_zlib.h")
#       # Set vars that FindZlib would have set if used in sub project
#       set (ZLIB_INCLUDE_DIRS "${VTK_ZLIB_INCLUDE_DIRS}")
#       set (ZLIB_LIBRARIES vtkzlib)
#     endif ()
#   endif ()
#
#   # Add the sub project
#   add_subdirectory (Utilities/hdf5-1.8)
#-----------------------------------------------------------------------------
string(TIMESTAMP CONFIG_DATE "%Y-%m-%d")

if (FALSE) # XXX(kitware): Hardcode settings.
#-----------------------------------------------------------------------------
# Allow Visual Studio solution directories
#-----------------------------------------------------------------------------
# Provide a way for Visual Studio Express users to turn OFF the new FOLDER
# organization feature. Default to ON for non-Express users. Express users must
# explicitly turn off this option to build HDF5 in the Express IDE...
#
option (HDF5_USE_FOLDERS "Enable folder grouping of projects in IDEs." ON)
mark_as_advanced (HDF5_USE_FOLDERS)
if (HDF5_USE_FOLDERS)
  set_property (GLOBAL PROPERTY USE_FOLDERS ON)
endif ()
option (HDF5_NO_PACKAGES "CPACK - Disable packaging" OFF)
mark_as_advanced (HDF5_NO_PACKAGES)
option (ALLOW_UNSUPPORTED "Allow unsupported combinations of configure options" OFF)
mark_as_advanced (ALLOW_UNSUPPORTED)
else ()
set(HDF5_USE_FOLDERS OFF)
set(HDF5_NO_PACKAGES ON)
set(ALLOW_UNSUPPORTED OFF)
endif ()

#-----------------------------------------------------------------------------
# Set the core names of all the libraries
#-----------------------------------------------------------------------------
set (HDF5_LIB_CORENAME              "hdf5")
set (HDF5_TEST_LIB_CORENAME         "hdf5_test")
set (HDF5_CPP_LIB_CORENAME          "hdf5_cpp")
set (HDF5_HL_LIB_CORENAME           "hdf5_hl")
set (HDF5_HL_CPP_LIB_CORENAME       "hdf5_hl_cpp")
set (HDF5_TOOLS_LIB_CORENAME        "hdf5_tools")
set (HDF5_F90_LIB_CORENAME          "hdf5_fortran")
set (HDF5_F90_C_LIB_CORENAME        "hdf5_f90cstub")
set (HDF5_F90_TEST_LIB_CORENAME     "hdf5_test_fortran")
set (HDF5_F90_C_TEST_LIB_CORENAME   "hdf5_test_f90cstub")
set (HDF5_HL_F90_LIB_CORENAME       "hdf5_hl_fortran")
set (HDF5_HL_F90_C_LIB_CORENAME     "hdf5_hl_f90cstub")
set (HDF5_JAVA_JNI_LIB_CORENAME     "hdf5_java")
set (HDF5_JAVA_HDF5_LIB_CORENAME    "jarhdf5")
set (HDF5_JAVA_TEST_LIB_CORENAME    "jartest5")

#-----------------------------------------------------------------------------
# Set the true names of all the libraries if customized by external project
#-----------------------------------------------------------------------------
set (HDF5_LIB_NAME              "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_LIB_CORENAME}")
set (HDF5_TEST_LIB_NAME         "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_TEST_LIB_CORENAME}")
set (HDF5_CPP_LIB_NAME          "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_CPP_LIB_CORENAME}")
set (HDF5_HL_LIB_NAME           "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_HL_LIB_CORENAME}")
set (HDF5_HL_CPP_LIB_NAME       "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_HL_CPP_LIB_CORENAME}")
set (HDF5_TOOLS_LIB_NAME        "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_TOOLS_LIB_CORENAME}")
set (HDF5_F90_LIB_NAME          "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_F90_LIB_CORENAME}")
set (HDF5_F90_C_LIB_NAME        "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_F90_C_LIB_CORENAME}")
set (HDF5_F90_TEST_LIB_NAME     "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_F90_TEST_LIB_CORENAME}")
set (HDF5_F90_C_TEST_LIB_NAME   "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_F90_C_TEST_LIB_CORENAME}")
set (HDF5_HL_F90_LIB_NAME       "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_HL_F90_LIB_CORENAME}")
set (HDF5_HL_F90_C_LIB_NAME     "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_HL_F90_C_LIB_CORENAME}")
set (HDF5_JAVA_JNI_LIB_NAME     "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_JAVA_JNI_LIB_CORENAME}")
set (HDF5_JAVA_HDF5_LIB_NAME    "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_JAVA_HDF5_LIB_CORENAME}")
set (HDF5_JAVA_TEST_LIB_NAME    "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_JAVA_TEST_LIB_CORENAME}")

#-----------------------------------------------------------------------------
# Set the target names of all the libraries
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Don't use `-static` or `-shared` target suffixes.
set (HDF5_LIB_TARGET              "${HDF5_LIB_CORENAME}-static")
set (HDF5_TEST_LIB_TARGET         "${HDF5_TEST_LIB_CORENAME}-static")
set (HDF5_CPP_LIB_TARGET          "${HDF5_CPP_LIB_CORENAME}-static")
set (HDF5_HL_LIB_TARGET           "${HDF5_HL_LIB_CORENAME}-static")
set (HDF5_HL_CPP_LIB_TARGET       "${HDF5_HL_CPP_LIB_CORENAME}-static")
set (HDF5_TOOLS_LIB_TARGET        "${HDF5_TOOLS_LIB_CORENAME}-static")
set (HDF5_F90_LIB_TARGET          "${HDF5_F90_LIB_CORENAME}-static")
set (HDF5_F90_C_LIB_TARGET        "${HDF5_F90_C_LIB_CORENAME}-static")
set (HDF5_F90_TEST_LIB_TARGET     "${HDF5_F90_TEST_LIB_CORENAME}-static")
set (HDF5_F90_C_TEST_LIB_TARGET   "${HDF5_F90_C_TEST_LIB_CORENAME}-static")
set (HDF5_HL_F90_LIB_TARGET       "${HDF5_HL_F90_LIB_CORENAME}-static")
set (HDF5_HL_F90_C_LIB_TARGET     "${HDF5_HL_F90_C_LIB_CORENAME}-static")
set (HDF5_JAVA_JNI_LIB_TARGET     "${HDF5_JAVA_JNI_LIB_CORENAME}")
set (HDF5_JAVA_HDF5_LIB_TARGET    "${HDF5_JAVA_HDF5_LIB_CORENAME}")
set (HDF5_JAVA_TEST_LIB_TARGET    "${HDF5_JAVA_TEST_LIB_CORENAME}")
set (HDF5_LIBSH_TARGET            "${HDF5_LIB_CORENAME}-shared")
set (HDF5_TEST_LIBSH_TARGET       "${HDF5_TEST_LIB_CORENAME}-shared")
set (HDF5_CPP_LIBSH_TARGET        "${HDF5_CPP_LIB_CORENAME}-shared")
set (HDF5_HL_LIBSH_TARGET         "${HDF5_HL_LIB_CORENAME}-shared")
set (HDF5_HL_CPP_LIBSH_TARGET     "${HDF5_HL_CPP_LIB_CORENAME}-shared")
set (HDF5_TOOLS_LIBSH_TARGET      "${HDF5_TOOLS_LIB_CORENAME}-shared")
set (HDF5_F90_LIBSH_TARGET        "${HDF5_F90_LIB_CORENAME}-shared")
set (HDF5_F90_C_LIBSH_TARGET      "${HDF5_F90_C_LIB_CORENAME}-shared")
set (HDF5_F90_TEST_LIBSH_TARGET   "${HDF5_F90_TEST_LIB_CORENAME}-shared")
set (HDF5_F90_C_TEST_LIBSH_TARGET "${HDF5_F90_C_TEST_LIB_CORENAME}-shared")
set (HDF5_HL_F90_LIBSH_TARGET     "${HDF5_HL_F90_LIB_CORENAME}-shared")
set (HDF5_HL_F90_C_LIBSH_TARGET   "${HDF5_HL_F90_C_LIB_CORENAME}-shared")
else ()
set (HDF5_LIB_TARGET              "${HDF5_LIB_NAME}")
set (HDF5_CPP_LIB_TARGET          "${HDF5_CPP_LIB_NAME}")
set (HDF5_HL_LIB_TARGET           "${HDF5_HL_LIB_NAME}")
set (HDF5_HL_CPP_LIB_TARGET       "${HDF5_HL_CPP_LIB_NAME}")
set (HDF5_TOOLS_LIB_TARGET        "${HDF5_TOOLS_LIB_NAME}")
set (HDF5_LIBSH_TARGET            "${HDF5_LIB_NAME}")
set (HDF5_CPP_LIBSH_TARGET        "${HDF5_CPP_LIB_NAME}")
set (HDF5_HL_LIBSH_TARGET         "${HDF5_HL_LIB_NAME}")
set (HDF5_HL_CPP_LIBSH_TARGET     "${HDF5_HL_CPP_LIB_NAME}")
set (HDF5_TOOLS_LIBSH_TARGET      "${HDF5_TOOLS_LIB_NAME}")
endif ()

#-----------------------------------------------------------------------------
# Define some CMake variables for use later in the project
#-----------------------------------------------------------------------------
set (HDF_RESOURCES_DIR       ${HDF5_SOURCE_DIR}/config/cmake)
set (HDF_RESOURCES_EXT_DIR   ${HDF5_SOURCE_DIR}/config/cmake_ext_mod)
set (HDF5_SRC_DIR            ${HDF5_SOURCE_DIR}/src)
set (HDF5_TEST_SRC_DIR       ${HDF5_SOURCE_DIR}/test)
set (HDF5_CPP_SRC_DIR        ${HDF5_SOURCE_DIR}/c++)
set (HDF5_CPP_TST_DIR        ${HDF5_SOURCE_DIR}/c++/test)
set (HDF5_HL_SRC_DIR         ${HDF5_SOURCE_DIR}/hl)
set (HDF5_HL_CPP_SRC_DIR     ${HDF5_SOURCE_DIR}/hl/c++)
set (HDF5_HL_TOOLS_DIR       ${HDF5_SOURCE_DIR}/hl/tools)
set (HDF5_TOOLS_DIR          ${HDF5_SOURCE_DIR}/tools)
set (HDF5_TOOLS_SRC_DIR      ${HDF5_SOURCE_DIR}/tools/src)
set (HDF5_PERFORM_SRC_DIR    ${HDF5_SOURCE_DIR}/tools/src/perform)
set (HDF5_F90_SRC_DIR        ${HDF5_SOURCE_DIR}/fortran)
set (HDF5_JAVA_JNI_SRC_DIR         ${HDF5_SOURCE_DIR}/java/src/jni)
set (HDF5_JAVA_HDF5_SRC_DIR        ${HDF5_SOURCE_DIR}/java/src/hdf)
set (HDF5_JAVA_TEST_SRC_DIR        ${HDF5_SOURCE_DIR}/java/test)
set (HDF5_JAVA_LIB_DIR             ${HDF5_SOURCE_DIR}/java/lib)
set (HDF5_JAVA_LOGGING_JAR         ${HDF5_SOURCE_DIR}/java/lib/slf4j-api-1.7.25.jar)
set (HDF5_JAVA_LOGGING_NOP_JAR     ${HDF5_SOURCE_DIR}/java/lib/ext/slf4j-nop-1.7.25.jar)
set (HDF5_JAVA_LOGGING_SIMPLE_JAR  ${HDF5_SOURCE_DIR}/java/lib/ext/slf4j-simple-1.7.25.jar)

#-----------------------------------------------------------------------------
# parse the full version number from H5public.h and include in H5_VERS_INFO
#-----------------------------------------------------------------------------
file (READ ${HDF5_SRC_DIR}/H5public.h _h5public_h_contents)
string (REGEX REPLACE ".*#define[ \t]+H5_VERS_MAJOR[ \t]+([0-9]*).*$"
    "\\1" H5_VERS_MAJOR ${_h5public_h_contents})
string (REGEX REPLACE ".*#define[ \t]+H5_VERS_MINOR[ \t]+([0-9]*).*$"
    "\\1" H5_VERS_MINOR ${_h5public_h_contents})
string (REGEX REPLACE ".*#define[ \t]+H5_VERS_RELEASE[ \t]+([0-9]*).*$"
    "\\1" H5_VERS_RELEASE ${_h5public_h_contents})
string (REGEX REPLACE ".*#define[ \t]+H5_VERS_SUBRELEASE[ \t]+\"([0-9A-Za-z._]*)\".*$"
    "\\1" H5_VERS_SUBRELEASE ${_h5public_h_contents})
#message (STATUS "VERSION: ${H5_VERS_MAJOR}.${H5_VERS_MINOR}.${H5_VERS_RELEASE}-${H5_VERS_SUBRELEASE}")

#-----------------------------------------------------------------------------
# parse the full soversion number from config/lt_vers.am and include in H5_SOVERS_INFO
#-----------------------------------------------------------------------------
file (READ ${HDF5_SOURCE_DIR}/config/lt_vers.am _lt_vers_am_contents)
string (REGEX REPLACE ".*LT_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$"
    "\\1" H5_LIB_SOVERS_INTERFACE ${_lt_vers_am_contents})
string (REGEX REPLACE ".*LT_VERS_REVISION[ \t]+=[ \t]+([0-9]*).*$"
    "\\1" H5_LIB_SOVERS_MINOR ${_lt_vers_am_contents})
string (REGEX REPLACE ".*LT_VERS_AGE[ \t]+=[ \t]+([0-9]*).*$"
    "\\1" H5_LIB_SOVERS_RELEASE ${_lt_vers_am_contents})
math (EXPR H5_LIB_SOVERS_MAJOR ${H5_LIB_SOVERS_INTERFACE}-${H5_LIB_SOVERS_RELEASE})
#message (STATUS "SOVERSION: ${H5_LIB_SOVERS_MAJOR}.${H5_LIB_SOVERS_RELEASE}.${H5_LIB_SOVERS_MINOR}")
string (REGEX MATCH ".*LT_TOOLS_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$" H5_TOOLS_SOVERS_EXISTS ${_lt_vers_am_contents})
if (H5_TOOLS_SOVERS_EXISTS)
  string (REGEX REPLACE ".*LT_TOOLS_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_TOOLS_SOVERS_INTERFACE ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_TOOLS_VERS_REVISION[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_TOOLS_SOVERS_MINOR ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_TOOLS_VERS_AGE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_TOOLS_SOVERS_RELEASE ${_lt_vers_am_contents})
  math (EXPR H5_TOOLS_SOVERS_MAJOR ${H5_TOOLS_SOVERS_INTERFACE}-${H5_TOOLS_SOVERS_RELEASE})
  #message (STATUS "SOVERSION_TOOLS: ${H5_TOOLS_SOVERS_MAJOR}.${H5_TOOLS_SOVERS_RELEASE}.${H5_TOOLS_SOVERS_MINOR}")
endif ()
string (REGEX MATCH ".*LT_CXX_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$" H5_CXX_SOVERS_EXISTS ${_lt_vers_am_contents})
if (H5_CXX_SOVERS_EXISTS)
  string (REGEX REPLACE ".*LT_CXX_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_CXX_SOVERS_INTERFACE ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_CXX_VERS_REVISION[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_CXX_SOVERS_MINOR ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_CXX_VERS_AGE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_CXX_SOVERS_RELEASE ${_lt_vers_am_contents})
  math (EXPR H5_CXX_SOVERS_MAJOR ${H5_CXX_SOVERS_INTERFACE}-${H5_CXX_SOVERS_RELEASE})
  #message (STATUS "SOVERSION_CXX: ${H5_CXX_SOVERS_MAJOR}.${H5_CXX_SOVERS_RELEASE}.${H5_CXX_SOVERS_MINOR}")
endif ()
string (REGEX MATCH ".*LT_F_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$" H5_F_SOVERS_EXISTS ${_lt_vers_am_contents})
if (H5_F_SOVERS_EXISTS AND FALSE) # XXX(kitware): VTK doesn't need Fortran support.
  string (REGEX REPLACE ".*LT_F_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_F_SOVERS_INTERFACE ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_F_VERS_REVISION[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_F_SOVERS_MINOR ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_F_VERS_AGE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_F_SOVERS_RELEASE ${_lt_vers_am_contents})
  math (EXPR H5_F_SOVERS_MAJOR ${H5_F_SOVERS_INTERFACE}-${H5_F_SOVERS_RELEASE})
  #message (STATUS "SOVERSION_F: ${H5_F_SOVERS_MAJOR}.${H5_F_SOVERS_RELEASE}.${H5_F_SOVERS_MINOR}")
endif ()
string (REGEX MATCH ".*LT_HL_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$" H5_HL_SOVERS_EXISTS ${_lt_vers_am_contents})
if (H5_HL_SOVERS_EXISTS)
  string (REGEX REPLACE ".*LT_HL_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_HL_SOVERS_INTERFACE ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_HL_VERS_REVISION[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_HL_SOVERS_MINOR ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_HL_VERS_AGE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_HL_SOVERS_RELEASE ${_lt_vers_am_contents})
  math (EXPR H5_HL_SOVERS_MAJOR ${H5_HL_SOVERS_INTERFACE}-${H5_HL_SOVERS_RELEASE})
  #message (STATUS "SOVERSION_HL: ${H5_HL_SOVERS_MAJOR}.${H5_HL_SOVERS_RELEASE}.${H5_HL_SOVERS_MINOR}")
endif ()
string (REGEX MATCH ".*LT_HL_CXX_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$" H5_HL_CXX_SOVERS_EXISTS ${_lt_vers_am_contents})
if (H5_HL_CXX_SOVERS_EXISTS)
  string (REGEX REPLACE ".*LT_HL_CXX_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_HL_CXX_SOVERS_INTERFACE ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_HL_CXX_VERS_REVISION[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_HL_CXX_SOVERS_MINOR ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_HL_CXX_VERS_AGE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_HL_CXX_SOVERS_RELEASE ${_lt_vers_am_contents})
  math (EXPR H5_HL_CXX_SOVERS_MAJOR ${H5_HL_CXX_SOVERS_INTERFACE}-${H5_HL_CXX_SOVERS_RELEASE})
  #message (STATUS "SOVERSION_HL_CXX: ${H5_HL_CXX_SOVERS_MAJOR}.${H5_HL_CXX_SOVERS_RELEASE}.${H5_HL_CXX_SOVERS_MINOR}")
endif ()
string (REGEX MATCH ".*LT_HL_F_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$" H5_HL_F_SOVERS_EXISTS ${_lt_vers_am_contents})
if (H5_HL_F_SOVERS_EXISTS AND FALSE) # XXX(kitware): VTK doesn't need Fortran support.
  string (REGEX REPLACE ".*LT_HL_F_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_HL_F_SOVERS_INTERFACE ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_HL_F_VERS_REVISION[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_HL_F_SOVERS_MINOR ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_HL_F_VERS_AGE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_HL_F_SOVERS_RELEASE ${_lt_vers_am_contents})
  math (EXPR H5_HL_F_SOVERS_MAJOR ${H5_HL_F_SOVERS_INTERFACE}-${H5_HL_F_SOVERS_RELEASE})
  message (STATUS "SOVERSION_HL_F: ${H5_HL_F_SOVERS_MAJOR}.${H5_HL_F_SOVERS_RELEASE}.${H5_HL_F_SOVERS_MINOR}")
endif ()
string (REGEX MATCH ".*LT_JAVA_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$" H5_JAVA_SOVERS_EXISTS ${_lt_vers_am_contents})
if(H5_JAVA_SOVERS_EXISTS AND FALSE) # XXX(kitware): VTK doesn't need Java support.
  string (REGEX REPLACE ".*LT_JAVA_VERS_INTERFACE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_JAVA_SOVERS_INTERFACE ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_JAVA_VERS_REVISION[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_JAVA_SOVERS_MINOR ${_lt_vers_am_contents})
  string (REGEX REPLACE ".*LT_JAVA_VERS_AGE[ \t]+=[ \t]+([0-9]*).*$"
      "\\1" H5_JAVA_SOVERS_RELEASE ${_lt_vers_am_contents})
  math (EXPR H5_JAVA_SOVERS_MAJOR ${H5_JAVA_SOVERS_INTERFACE}-${H5_JAVA_SOVERS_RELEASE})
  message (STATUS "SOVERSION_JAVA: ${H5_JAVA_SOVERS_MAJOR}.${H5_JAVA_SOVERS_RELEASE}.${H5_JAVA_SOVERS_MINOR}")
endif ()

#-----------------------------------------------------------------------------
# Basic HDF5 stuff here
#-----------------------------------------------------------------------------
set (HDF5_PACKAGE "hdf5")
set (HDF5_PACKAGE_NAME "HDF5")
set (HDF5_PACKAGE_VERSION "${H5_VERS_MAJOR}.${H5_VERS_MINOR}.${H5_VERS_RELEASE}")
set (HDF5_PACKAGE_VERSION_MAJOR "${H5_VERS_MAJOR}.${H5_VERS_MINOR}")
set (HDF5_PACKAGE_VERSION_MINOR "${H5_VERS_RELEASE}")
if (H5_VERS_SUBRELEASE)
  set (HDF5_PACKAGE_VERSION_STRING "${HDF5_PACKAGE_VERSION}-${H5_VERS_SUBRELEASE}")
else ()
  set (HDF5_PACKAGE_VERSION_STRING "${HDF5_PACKAGE_VERSION}")
endif ()
set (HDF5_LIB_PACKAGE_SOVERSION "${H5_LIB_SOVERS_MAJOR}.${H5_LIB_SOVERS_RELEASE}.${H5_LIB_SOVERS_MINOR}")
set (HDF5_LIB_PACKAGE_SOVERSION_MAJOR "${H5_LIB_SOVERS_MAJOR}")
if (H5_TOOLS_SOVERS_EXISTS)
  set (HDF5_TOOLS_PACKAGE_SOVERSION "${H5_TOOLS_SOVERS_MAJOR}.${H5_TOOLS_SOVERS_RELEASE}.${H5_TOOLS_SOVERS_MINOR}")
  set (HDF5_TOOLS_PACKAGE_SOVERSION_MAJOR "${H5_TOOLS_SOVERS_MAJOR}")
else ()
  set (HDF5_TOOLS_PACKAGE_SOVERSION "${H5_LIB_SOVERS_MAJOR}.${H5_LIB_SOVERS_RELEASE}.${H5_LIB_SOVERS_MINOR}")
  set (HDF5_TOOLS_PACKAGE_SOVERSION_MAJOR "${H5_LIB_SOVERS_MAJOR}")
endif ()
if (H5_CXX_SOVERS_EXISTS)
  set (HDF5_CXX_PACKAGE_SOVERSION "${H5_CXX_SOVERS_MAJOR}.${H5_CXX_SOVERS_RELEASE}.${H5_CXX_SOVERS_MINOR}")
  set (HDF5_CXX_PACKAGE_SOVERSION_MAJOR "${H5_CXX_SOVERS_MAJOR}")
else ()
  set (HDF5_CXX_PACKAGE_SOVERSION "${H5_LIB_SOVERS_MAJOR}.${H5_LIB_SOVERS_RELEASE}.${H5_LIB_SOVERS_MINOR}")
  set (HDF5_CXX_PACKAGE_SOVERSION_MAJOR "${H5_LIB_SOVERS_MAJOR}")
endif ()
if (H5_F_SOVERS_EXISTS)
  set (HDF5_F_PACKAGE_SOVERSION "${H5_F_SOVERS_MAJOR}.${H5_F_SOVERS_RELEASE}.${H5_F_SOVERS_MINOR}")
  set (HDF5_F_PACKAGE_SOVERSION_MAJOR "${H5_F_SOVERS_MAJOR}")
else ()
  set (HDF5_F_PACKAGE_SOVERSION "${H5_LIB_SOVERS_MAJOR}.${H5_LIB_SOVERS_RELEASE}.${H5_LIB_SOVERS_MINOR}")
  set (HDF5_F_PACKAGE_SOVERSION_MAJOR "${H5_LIB_SOVERS_MAJOR}")
endif ()
if (H5_HL_SOVERS_EXISTS)
  set (HDF5_HL_PACKAGE_SOVERSION "${H5_HL_SOVERS_MAJOR}.${H5_HL_SOVERS_RELEASE}.${H5_HL_SOVERS_MINOR}")
  set (HDF5_HL_PACKAGE_SOVERSION_MAJOR "${H5_HL_SOVERS_MAJOR}")
else ()
  set (HDF5_HL_PACKAGE_SOVERSION "${H5_LIB_SOVERS_MAJOR}.${H5_LIB_SOVERS_RELEASE}.${H5_LIB_SOVERS_MINOR}")
  set (HDF5_HL_PACKAGE_SOVERSION_MAJOR "${H5_LIB_SOVERS_MAJOR}")
endif ()
if (H5_HL_F_SOVERS_EXISTS)
  set (HDF5_HL_CXX_PACKAGE_SOVERSION "${H5_HL_CXX_SOVERS_MAJOR}.${H5_HL_CXX_SOVERS_RELEASE}.${H5_HL_CXX_SOVERS_MINOR}")
  set (HDF5_HL_CXX_PACKAGE_SOVERSION_MAJOR "${H5_HL_CXX_SOVERS_MAJOR}")
else ()
  set (HDF5_HL_CXX_PACKAGE_SOVERSION "${H5_LIB_SOVERS_MAJOR}.${H5_LIB_SOVERS_RELEASE}.${H5_LIB_SOVERS_MINOR}")
  set (HDF5_HL_CXX_PACKAGE_SOVERSION_MAJOR "${H5_LIB_SOVERS_MAJOR}")
endif ()
if (H5_HL_F_SOVERS_EXISTS)
  set (HDF5_HL_F_PACKAGE_SOVERSION "${H5_HL_F_SOVERS_MAJOR}.${H5_HL_F_SOVERS_RELEASE}.${H5_HL_F_SOVERS_MINOR}")
  set (HDF5_HL_F_PACKAGE_SOVERSION_MAJOR "${H5_HL_F_SOVERS_MAJOR}")
else ()
  set (HDF5_HL_F_PACKAGE_SOVERSION "${H5_LIB_SOVERS_MAJOR}.${H5_LIB_SOVERS_RELEASE}.${H5_LIB_SOVERS_MINOR}")
  set (HDF5_HL_F_PACKAGE_SOVERSION_MAJOR "${H5_LIB_SOVERS_MAJOR}")
endif ()
if (H5_JAVA_SOVERS_EXISTS)
  set (HDF5_JAVA_PACKAGE_SOVERSION "${H5_JAVA_SOVERS_MAJOR}.${H5_JAVA_SOVERS_RELEASE}.${H5_JAVA_SOVERS_MINOR}")
  set (HDF5_JAVA_PACKAGE_SOVERSION_MAJOR "${H5_JAVA_SOVERS_MAJOR}")
else ()
  set (HDF5_JAVA_PACKAGE_SOVERSION "${H5_LIB_SOVERS_MAJOR}.${H5_LIB_SOVERS_RELEASE}.${H5_LIB_SOVERS_MINOR}")
  set (HDF5_JAVA_PACKAGE_SOVERSION_MAJOR "${H5_LIB_SOVERS_MAJOR}")
endif ()
set (HDF5_PACKAGE_STRING "${HDF5_PACKAGE_NAME} ${HDF5_PACKAGE_VERSION_STRING}")
set (HDF5_PACKAGE_TARNAME "${HDF5_PACKAGE}${HDF_PACKAGE_EXT}")
set (HDF5_PACKAGE_URL "http://www.hdfgroup.org")
set (HDF5_PACKAGE_BUGREPORT "help@hdfgroup.org")

#-----------------------------------------------------------------------------
# Include some macros for reusable code
#-----------------------------------------------------------------------------
include (${HDF_RESOURCES_EXT_DIR}/HDFMacros.cmake)

HDF_DIR_PATHS(${HDF5_PACKAGE_NAME})

include (${HDF_RESOURCES_EXT_DIR}/HDFLibMacros.cmake)
include (${HDF_RESOURCES_DIR}/HDF5Macros.cmake)

#-----------------------------------------------------------------------------
# Targets built within this project are exported at Install time for use
# by other projects.
#-----------------------------------------------------------------------------
set(HDF5_EXPORTED_TARGETS "${_vtk_build_INSTALL_EXPORT}")
if (NOT HDF5_EXPORTED_TARGETS)
  set (HDF5_EXPORTED_TARGETS "hdf5-targets")
endif ()

#-----------------------------------------------------------------------------
# To include a library in the list exported by the project AT BUILD TIME,
# add it to this variable. This is NOT used by Make Install, but for projects
# which include hdf5 as a sub-project within their build tree
#-----------------------------------------------------------------------------
set_global_variable (HDF5_LIBRARIES_TO_EXPORT "")
set_global_variable (HDF5_UTILS_TO_EXPORT "")

set (EXTERNAL_HEADER_LIST "")
set (EXTERNAL_LIBRARY_LIST "")
set (EXTERNAL_LIBRARYDLL_LIST "")

#-----------------------------------------------------------------------------
# Run all the CMake configuration tests for our build environment
#-----------------------------------------------------------------------------
include (${HDF_RESOURCES_DIR}/ConfigureChecks.cmake)

set (CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

#-----------------------------------------------------------------------------
# Mac OS X Options
#-----------------------------------------------------------------------------
if (HDF5_BUILD_FRAMEWORKS AND NOT BUILD_SHARED_LIBS)
  set (BUILD_SHARED_LIBS ON CACHE BOOL "Build Shared Libraries" FORCE)
endif ()

#-----------------------------------------------------------------------------
# Option to Build Shared and Static libs, default is both
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): `BUILD_SHARED_LIBS` is handled by VTK.
option (ONLY_SHARED_LIBS "Only Build Shared Libraries" OFF)
set (H5_ENABLE_STATIC_LIB YES)
if (ONLY_SHARED_LIBS)
  set (H5_ENABLE_STATIC_LIB NO)
  set (BUILD_SHARED_LIBS ON CACHE BOOL "Build Shared Libraries")
endif ()
option (BUILD_SHARED_LIBS "Build Shared Libraries" ON)
else ()
set(ONLY_SHARED_LIBS ON)
set (H5_ENABLE_STATIC_LIB NO)
endif ()
set (H5_ENABLE_SHARED_LIB NO)
if (BUILD_SHARED_LIBS)
  set (H5_ENABLE_SHARED_LIB YES)
endif ()
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

#-----------------------------------------------------------------------------
# Option to Build Static executables
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (BUILD_STATIC_EXECS "Build Static Executables" OFF)
else ()
set(BUILD_STATIC_EXECS OFF)
endif ()
if (BUILD_STATIC_EXECS)
  if (NOT WIN32)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
  endif ()
endif ()
set(H5_ENABLE_SHARED_LIB ${BUILD_SHARED_LIBS})
if (NOT BUILD_SHARED_LIBS)
set(H5_ENABLE_STATIC_LIB ON)
else ()
set(H5_ENABLE_STATIC_LIB OFF)
endif ()

if (FALSE) # XXX(kitware): Hide settings VTK doesn't need.
#-----------------------------------------------------------------------------
# Option to use code coverage
#-----------------------------------------------------------------------------
option (HDF5_ENABLE_COVERAGE "Enable code coverage for Libraries and Programs" OFF)
if (HDF5_ENABLE_COVERAGE)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage -fprofile-arcs -ftest-coverage")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g --coverage -O0 -fprofile-arcs -ftest-coverage")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set (LDFLAGS "${LDFLAGS} -fprofile-arcs -ftest-coverage")
    link_libraries (gcov)
  else ()
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to indicate using dmalloc
#-----------------------------------------------------------------------------
# option (HDF5_ENABLE_USING_DMALLOC "Indicate that dmalloc is used" OFF)
# if (HDF5_ENABLE_USING_DMALLOC)
#   find_package (DMALLOC)
#   set (H5_HAVE_DMALLOC DMALLOC_FOUND)
# endif ()

#-----------------------------------------------------------------------------
# Option to indicate using a memory checker
#-----------------------------------------------------------------------------
option (HDF5_ENABLE_USING_MEMCHECKER "Indicate that a memory checker is used" OFF)
if (HDF5_ENABLE_USING_MEMCHECKER)
  set (H5_USING_MEMCHECKER 1)
endif ()

#-----------------------------------------------------------------------------
# Option to indicate internal memory allocation sanity checks are enabled
#-----------------------------------------------------------------------------
option (HDF5_MEMORY_ALLOC_SANITY_CHECK "Indicate that internal memory allocation sanity checks are enabled" OFF)
if (HDF5_MEMORY_ALLOC_SANITY_CHECK)
  set (H5_MEMORY_ALLOC_SANITY_CHECK 1)
endif ()

#-----------------------------------------------------------------------------
# Option to enable/disable using pread/pwrite for VFDs
#-----------------------------------------------------------------------------
option (HDF5_ENABLE_PREADWRITE "Use pread/pwrite in sec2/log/core VFDs in place of read/write (when available)" ON)
mark_as_advanced (HDF5_ENABLE_PREADWRITE)
if (HDF5_ENABLE_PREADWRITE AND H5_HAVE_PREAD AND H5_HAVE_PWRITE)
  set (H5_HAVE_PREADWRITE 1)
endif ()

#-----------------------------------------------------------------------------
# Option to use deprecated public API symbols
#-----------------------------------------------------------------------------
option (HDF5_ENABLE_DEPRECATED_SYMBOLS "Enable deprecated public API symbols" ON)
if (HDF5_ENABLE_DEPRECATED_SYMBOLS)
  set (H5_NO_DEPRECATED_SYMBOLS 0)
else ()
  set (H5_NO_DEPRECATED_SYMBOLS 1)
endif ()
else ()
set(HDF5_ENABLE_COVERAGE OFF)
set(HDF5_ENABLE_USING_MEMCHECKER OFF)
set(HDF5_MEMORY_ALLOC_SANITY_CHECK OFF)
set(HDF5_ENABLE_DEPRECATED_SYMBOLS OFF)
set(H5_NO_DEPRECATED_SYMBOLS 0)
endif ()

#-----------------------------------------------------------------------------
# When building utility executables that generate other (source) files :
# we make use of the following variables defined in the root CMakeLists.
# Certain systems may add /Debug or /Release to output paths
# and we need to call the executable from inside the CMake configuration
#-----------------------------------------------------------------------------
set (EXE_EXT "")
if (WIN32 OR MINGW)
  set (EXE_EXT ".exe")
  if (FALSE) # XXX(kitware): this is not what we want for VTK.
  add_definitions (-D_BIND_TO_CURRENT_VCLIBS_VERSION=1)
  endif ()
  add_definitions (-D_CRT_SECURE_NO_WARNINGS)
  add_definitions (-D_CONSOLE)
endif ()

if (MSVC)
  set (CMAKE_MFC_FLAG 0)
  set (WIN_COMPILE_FLAGS "")
  set (WIN_LINK_FLAGS "")
endif ()

set (MAKE_SYSTEM)
if (CMAKE_BUILD_TOOL MATCHES "make")
  set (MAKE_SYSTEM 1)
endif ()

set (CFG_INIT "/${CMAKE_CFG_INTDIR}")
if (MAKE_SYSTEM)
  set (CFG_INIT "")
endif ()

if (FALSE) # XXX(kitware): Hide settings VTK doesn't need.
# Do not generate test programs by default
option (HDF5_BUILD_GENERATORS "Build Test Generators" OFF)

#-----------------------------------------------------------------------------
# Add some definitions for Debug Builds
#-----------------------------------------------------------------------------
option (HDF5_ENABLE_TRACE "Enable API tracing capability" OFF)
mark_as_advanced (HDF5_ENABLE_TRACE)
if (${HDF_CFG_NAME} MATCHES "Debug")
  # Enable instrumenting of the library's internal operations
  option (HDF5_ENABLE_INSTRUMENT "Instrument The library" OFF)
  if (HDF5_ENABLE_INSTRUMENT)
    set (H5_HAVE_INSTRUMENTED_LIBRARY 1)
  endif ()
  mark_as_advanced (HDF5_ENABLE_INSTRUMENT)
endif ()
else ()
set(HDF5_BUILD_GENERATORS OFF)
set(HDF5_ENABLE_TRACE OFF)
endif ()

#-----------------------------------------------------------------------------
# Option to embed library info into executables
#-----------------------------------------------------------------------------
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set (H5_HAVE_EMBEDDED_LIBINFO 0)
else ()
  if (FALSE) # XXX(kitware): Hardcode settings.
  option (HDF5_ENABLE_EMBEDDED_LIBINFO "embed library info into executables" ON)
  if (HDF5_ENABLE_EMBEDDED_LIBINFO)
    set (H5_HAVE_EMBEDDED_LIBINFO 1)
  endif ()
  else ()
  set(HDF5_ENABLE_EMBEDDED_LIBINFO 1)
  endif ()
endif ()

include (${HDF_RESOURCES_DIR}/HDFCompilerFlags.cmake)
set (CMAKE_MODULE_PATH ${HDF_RESOURCES_DIR} ${HDF_RESOURCES_EXT_DIR} ${CMAKE_MODULE_PATH})

#-----------------------------------------------------------------------------
# Option to Enable HDFS
#-----------------------------------------------------------------------------
option (HDF5_ENABLE_HDFS "Enable HDFS" OFF)
if (HDF5_ENABLE_HDFS)
  find_package(JNI REQUIRED)
  if (JNI_FOUND)
    set (H5_HAVE_LIBJVM 1)
  endif ()
  find_package(HDFS REQUIRED)
  if (HDFS_FOUND)
    set (H5_HAVE_LIBHDFS 1)
    set (H5_HAVE_HDFS_H 1)
    if (NOT MSVC)
      list (APPEND LINK_LIBS -pthread)
    endif ()
  else ()
    message (FATAL_ERROR "Set to use libhdfs library, but could not find or use libhdfs. Please verify that the path to HADOOP_HOME is valid, and/or reconfigure without HDF5_ENABLE_HDFS")
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to Enable MPI Parallel
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_PARALLEL "Enable parallel build (requires MPI)" OFF)
if (HDF5_ENABLE_PARALLEL)
  find_package(MPI REQUIRED)
  if (MPI_C_FOUND)
    set (H5_HAVE_PARALLEL 1)
    if (FALSE) # XXX(kitware): VTK doesn't need Fortran support.
    # MPI checks, only do these if MPI_C_FOUND is true, otherwise they always fail
    # and once set, they are cached as false and not regenerated
    set (CMAKE_REQUIRED_LIBRARIES "${MPI_C_LIBRARIES}")
    set (CMAKE_REQUIRED_INCLUDES "${MPI_C_INCLUDE_DIRS}")
    # Used by Fortran + MPI
    CHECK_SYMBOL_EXISTS (MPI_Comm_c2f "mpi.h"  H5_HAVE_MPI_MULTI_LANG_Comm)
    CHECK_SYMBOL_EXISTS (MPI_Info_c2f "mpi.h"  H5_HAVE_MPI_MULTI_LANG_Info)
  endif ()

    # Used by Parallel Compression feature
    set (PARALLEL_FILTERED_WRITES ON)
    CHECK_SYMBOL_EXISTS (MPI_Mprobe "mpi.h" H5_HAVE_MPI_Mprobe)
    CHECK_SYMBOL_EXISTS (MPI_Imrecv "mpi.h" H5_HAVE_MPI_Imrecv)
    if (NOT H5_HAVE_MPI_Mprobe OR NOT H5_HAVE_MPI_Imrecv)
      message (WARNING "The MPI_Mprobe and/or MPI_Imrecv functions could not be located.
               Parallel writes of filtered data will be disabled.")
      set (PARALLEL_FILTERED_WRITES OFF)
    endif ()

    # Used by big I/O feature
    set (LARGE_PARALLEL_IO ON)
    CHECK_SYMBOL_EXISTS (MPI_Get_elements_x "mpi.h" H5_HAVE_MPI_Get_elements_x)
    CHECK_SYMBOL_EXISTS (MPI_Type_size_x "mpi.h" H5_HAVE_MPI_Type_size_x)
    if (NOT H5_HAVE_MPI_Get_elements_x OR NOT H5_HAVE_MPI_Type_size_x)
      message (WARNING "The MPI_Get_elements_x and/or MPI_Type_size_x functions could not be located.
               Reading/Writing >2GB of data in a single parallel I/O operation will be disabled.")
      set (LARGE_PARALLEL_IO OFF)
    endif ()
  else ()
    message (FATAL_ERROR "Parallel libraries not found")
  endif ()
endif ()

# Parallel IO usage requires MPI to be Linked and Included
if (H5_HAVE_PARALLEL)
  set (LINK_LIBS ${LINK_LIBS} ${MPI_C_LIBRARIES})
  if (MPI_C_LINK_FLAGS)
    set (CMAKE_EXE_LINKER_FLAGS "${MPI_C_LINK_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
  endif ()
endif ()
else ()
set(HDF5_ENABLE_PARALLEL OFF)
endif ()

if (FALSE) # XXX(kitware): Hardcode settings.
#-----------------------------------------------------------------------------
# Option to use 1.6.x API
#-----------------------------------------------------------------------------
set (H5_USE_16_API_DEFAULT 0)
if (DEFAULT_API_VERSION MATCHES "v16")
  set (H5_USE_16_API_DEFAULT 1)
endif ()

#-----------------------------------------------------------------------------
# Option to use 1.8.x API
#-----------------------------------------------------------------------------
set (H5_USE_18_API_DEFAULT 0)
if (DEFAULT_API_VERSION MATCHES "v18")
  set (H5_USE_18_API_DEFAULT 1)
endif ()

#-----------------------------------------------------------------------------
# Option to use 1.10.x API
#-----------------------------------------------------------------------------
if (NOT DEFAULT_API_VERSION)
  set (DEFAULT_API_VERSION "v110")
endif ()
set (H5_USE_110_API_DEFAULT 0)
if (DEFAULT_API_VERSION MATCHES "v110")
  set (H5_USE_110_API_DEFAULT 1)
endif ()
else ()
set (H5_USE_16_API_DEFAULT 0)
set (H5_USE_18_API_DEFAULT 0)
set (H5_USE_110_API_DEFAULT 1)
set (DEFAULT_API_VERSION "v110")
endif ()

#-----------------------------------------------------------------------------
# Include user macros
#-----------------------------------------------------------------------------
include (UserMacros.cmake)

#-----------------------------------------------------------------------------
# Include filter (zlib, szip, etc.) macros
#-----------------------------------------------------------------------------
include (CMakeFilters.cmake)

if (FALSE) # XXX(kitware): Disable packaging.
#-----------------------------------------------------------------------------
# Option for external libraries on windows
#-----------------------------------------------------------------------------
option (HDF5_PACKAGE_EXTLIBS "CPACK - include external libraries" OFF)
if (NOT HDF5_EXTERNALLY_CONFIGURED)
  if (HDF5_PACKAGE_EXTLIBS)
    set (HDF5_NO_PACKAGES OFF CACHE BOOL "CPACK - Disable packaging" FORCE)
    if (HDF5_ENABLE_Z_LIB_SUPPORT AND ZLIB_FOUND)
      PACKAGE_ZLIB_LIBRARY (${HDF5_ALLOW_EXTERNAL_SUPPORT})
    endif ()

    if (HDF5_ENABLE_SZIP_SUPPORT AND SZIP_FOUND)
      PACKAGE_SZIP_LIBRARY (${HDF5_ALLOW_EXTERNAL_SUPPORT})
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to use threadsafe
#-----------------------------------------------------------------------------
option (HDF5_ENABLE_THREADSAFE "Enable thread-safety" OFF)
if (HDF5_ENABLE_THREADSAFE)
  # check for unsupported options
  if (WIN32 OR MINGW)
    message (STATUS " **** thread-safety option not supported with static library **** ")
    message (STATUS " **** thread-safety option will not be used building static library **** ")
  endif ()
  if (HDF5_ENABLE_PARALLEL)
    if (NOT ALLOW_UNSUPPORTED)
      message (FATAL_ERROR " **** parallel and thread-safety options are not supported **** ")
    else ()
      message (STATUS " **** Allowing unsupported parallel and thread-safety options **** ")
    endif ()
  endif ()
  if (HDF5_BUILD_FORTRAN)
    if (NOT ALLOW_UNSUPPORTED)
      message (FATAL_ERROR " **** Fortran and thread-safety options are not supported **** ")
    else ()
      message (STATUS " **** Allowing unsupported Fortran and thread-safety options **** ")
    endif ()
  endif ()
  if (HDF5_BUILD_CPP_LIB)
    if (NOT ALLOW_UNSUPPORTED)
      message (FATAL_ERROR " **** C++ and thread-safety options are not supported **** ")
    else ()
      message (STATUS " **** Allowing unsupported C++ and thread-safety options **** ")
    endif ()
  endif ()
  if (HDF5_BUILD_HL_LIB)
    if (NOT ALLOW_UNSUPPORTED)
      message (FATAL_ERROR " **** HL and thread-safety options are not supported **** ")
    else ()
      message (STATUS " **** Allowing unsupported HL and thread-safety options **** ")
    endif ()
  endif ()
  if (H5_HAVE_IOEO)
    message (STATUS " **** Win32 threads requires WINVER>=0x600 (Windows Vista/7/8) **** ")
    set (H5_HAVE_WIN_THREADS 1)
  else ()
    if (NOT H5_HAVE_PTHREAD_H)
      message (FATAL_ERROR " **** thread-safe option requires Win32 threads or Pthreads **** ")
    endif ()
  endif ()
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  if (Threads_FOUND)
    set (H5_HAVE_THREADSAFE 1)
  endif ()
endif ()
else ()
set(HDF5_ENABLE_THREADSAFE OFF)
endif ()

#-----------------------------------------------------------------------------
# Add the HDF5 Library Target to the build
#-----------------------------------------------------------------------------
add_subdirectory (src)

if (FALSE) # XXX(kitware): Hardcode settings.
if (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT" OR HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
  if (ZLIB_FOUND AND ZLIB_USE_EXTERNAL)
    if (NOT ONLY_SHARED_LIBS)
      add_dependencies (${HDF5_LIB_TARGET} ZLIB)
    endif ()
    if (BUILD_SHARED_LIBS)
      add_dependencies (${HDF5_LIBSH_TARGET} ZLIB)
    endif ()
  endif ()
  if (SZIP_FOUND AND SZIP_USE_EXTERNAL)
    if (NOT ONLY_SHARED_LIBS)
      add_dependencies (${HDF5_LIB_TARGET} SZIP)
    endif ()
    if (BUILD_SHARED_LIBS)
      add_dependencies (${HDF5_LIBSH_TARGET} SZIP)
    endif ()
  endif ()
endif ()
endif ()

if (FALSE) # XXX(kitware): Disable testing of HDF5.
#-----------------------------------------------------------------------------
# Dashboard and Testing Settings
#-----------------------------------------------------------------------------
option (BUILD_TESTING "Build HDF5 Unit Testing" ON)
if (BUILD_TESTING)
  set (DART_TESTING_TIMEOUT 1200
      CACHE STRING
      "Timeout in seconds for each test (default 1200=20minutes)"
  )

  # Generate a list of timeouts based on DART_TESTING_TIMEOUT
  math (EXPR CTEST_SHORT_TIMEOUT "${DART_TESTING_TIMEOUT} / 2")
  math (EXPR CTEST_LONG_TIMEOUT "${DART_TESTING_TIMEOUT} * 2")
  math (EXPR CTEST_VERY_LONG_TIMEOUT "${DART_TESTING_TIMEOUT} * 3")

  option (HDF5_TEST_VFD "Execute tests with different VFDs" OFF)
  mark_as_advanced (HDF5_TEST_VFD)
  if (HDF5_TEST_VFD)
    option (HDF5_TEST_FHEAP_VFD "Execute tests with different VFDs" ON)
    mark_as_advanced (HDF5_TEST_FHEAP_VFD)
  endif ()

  option (HDF_TEST_EXPRESS "Control testing framework (0-3)" "0")
  mark_as_advanced (HDF_TEST_EXPRESS)

  enable_testing ()
  include (CTest)

  include (${HDF5_SOURCE_DIR}/CTestConfig.cmake)
  configure_file (${HDF_RESOURCES_DIR}/CTestCustom.cmake ${HDF5_BINARY_DIR}/CTestCustom.ctest @ONLY)

  if (NOT HDF5_EXTERNALLY_CONFIGURED)
    if (EXISTS "${HDF5_SOURCE_DIR}/test" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/test")
      add_subdirectory (test)
    endif ()
    if (H5_HAVE_PARALLEL)
      if (EXISTS "${HDF5_SOURCE_DIR}/testpar" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/testpar")
        add_subdirectory (testpar)
      endif ()
    endif ()
  endif ()
endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build HDF5 Tools
#-----------------------------------------------------------------------------
if (EXISTS "${HDF5_SOURCE_DIR}/tools" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/tools")
  if (FALSE) # XXX(kitware): Hardcode settings.
  option (HDF5_BUILD_TOOLS  "Build HDF5 Tools" ON)
  else ()
  set(HDF5_BUILD_TOOLS OFF)
  endif ()
  if (HDF5_BUILD_TOOLS)
    add_subdirectory (tools)
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build examples
#-----------------------------------------------------------------------------
if (EXISTS "${HDF5_SOURCE_DIR}/examples" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/examples")
  if (FALSE) # XXX(kitware): Hardcode settings.
  option (HDF5_BUILD_EXAMPLES  "Build HDF5 Library Examples" ON)
  else ()
  set(HDF5_BUILD_EXAMPLES OFF)
  endif ()
  if (HDF5_BUILD_EXAMPLES)
    add_subdirectory (examples)
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build High Level API's
#-----------------------------------------------------------------------------
if (EXISTS "${HDF5_SOURCE_DIR}/hl" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/hl")
  if (FALSE) # XXX(kitware): Hardcode settings.
  option (HDF5_BUILD_HL_LIB  "Build HIGH Level HDF5 Library" ON)
  else ()
  set(HDF5_BUILD_HL_LIB ON)
  endif ()
  if (HDF5_BUILD_HL_LIB)
    set (H5_INCLUDE_HL 1)
    add_subdirectory (hl)
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build Fortran bindings/tests/examples
# Make sure this appears before the CONFIGURE_FILE step
# so that fortran name mangling is detected before writing H5pubconf.h
#-----------------------------------------------------------------------------
# Set default name mangling : overridden by Fortran detection in fortran dir
set (H5_FC_FUNC  "H5_FC_FUNC(name,NAME) name ## _")
set (H5_FC_FUNC_ "H5_FC_FUNC_(name,NAME) name ## _")
# XXX(kitware): VTK doesn't need Fortran support.
if (EXISTS "${HDF5_SOURCE_DIR}/fortran" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/fortran" AND FALSE)
  option (HDF5_BUILD_FORTRAN "Build FORTRAN support" OFF)
  if (HDF5_BUILD_FORTRAN)
    include (${HDF_RESOURCES_EXT_DIR}/HDFUseFortran.cmake)

    message (STATUS "Fortran compiler ID is ${CMAKE_Fortran_COMPILER_ID}")
    # Building with PGI requires CMake 3.3 or greater  because previous versions
    # of CMake add the wrong compiler flag for the PGI Fortran compiler.
    if (CMAKE_Fortran_COMPILER_ID MATCHES "PGI" AND CMAKE_VERSION VERSION_LESS "3.3")
      message (FATAL_ERROR " **** PGI FORTRAN REQUIRES CMAKE VERSION 3.3 OR GREATER **** ")
    endif ()

    include (${HDF_RESOURCES_DIR}/HDF5UseFortran.cmake)
    set (LINK_Fortran_LIBS ${LINK_LIBS})

    if (NOT H5_FORTRAN_HAVE_ISO_C_BINDING)
      message (FATAL_ERROR " **** Fortran compiler lacks required Fortran 2003 features; unsupported Fortran 2003 compiler, disable HDF5_BUILD_FORTRAN **** ")
    endif ()

    # Parallel IO usage requires MPI to be Linked and Included
    if (H5_HAVE_PARALLEL)
      set (LINK_Fortran_LIBS ${LINK_Fortran_LIBS} ${MPI_Fortran_LIBRARIES})
      if (MPI_Fortran_LINK_FLAGS)
        set (CMAKE_Fortran_EXE_LINKER_FLAGS "${MPI_Fortran_LINK_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
      endif ()
    endif ()

    add_subdirectory (fortran)
    if (HDF5_BUILD_HL_LIB)
      if (EXISTS "${HDF5_SOURCE_DIR}/hl/fortran" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/hl/fortran")
        #-- Build the High Level Fortran source codes
        add_subdirectory (hl/fortran)
      endif ()
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build HDF5 C++ Library
#-----------------------------------------------------------------------------
if (EXISTS "${HDF5_SOURCE_DIR}/c++" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/c++")
  if (FALSE) # XXX(kitware): Hardcode settings.
  option (HDF5_BUILD_CPP_LIB  "Build HDF5 C++ Library" ON)
  else ()
  set(HDF5_BUILD_CPP_LIB OFF)
  endif ()
  if (HDF5_BUILD_CPP_LIB)
    # check for unsupported options
    if (HDF5_ENABLE_PARALLEL)
      if (NOT ALLOW_UNSUPPORTED)
        message (FATAL_ERROR " **** Parallel and C++ options are mutually exclusive **** ")
      else ()
        message (STATUS " **** Allowing unsupported Parallel and C++ options **** ")
      endif ()
    endif ()

    include (${HDF_RESOURCES_EXT_DIR}/HDFUseCXX.cmake)

    if (CMAKE_NO_STD_NAMESPACE)
      set (H5_NO_STD 1)
    endif ()
    add_subdirectory (c++)
    if (HDF5_BUILD_HL_LIB)
      if (EXISTS "${HDF5_SOURCE_DIR}/hl/c++" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/hl/c++")
        #-- Build the High Level Fortran source codes
        add_subdirectory (hl/c++)
      endif ()
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Check if Fortran's default real is double precision. If it is and HL is
# being built then configure should fail due to bug HDFFV-889.
#-----------------------------------------------------------------------------
if (HDF5_BUILD_FORTRAN AND HDF5_BUILD_HL_LIB)
  if (NOT H5_FORTRAN_DEFAULT_REAL_NOT_DOUBLE)
    message (FATAL_ERROR " **** Fortran high-level routines are not supported when the default REAL is DOUBLE PRECISION, use HDF5_BUILD_HL_LIB:BOOL=OFF **** ")
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build HDF5 Java Library
#-----------------------------------------------------------------------------
# XXX(kitware): VTK doesn't need Java support.
if (EXISTS "${HDF5_SOURCE_DIR}/java" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/java" AND FALSE)
  option (HDF5_BUILD_JAVA  "Build Java HDF5 Library" OFF)
  if (HDF5_BUILD_JAVA)
    add_subdirectory (java)
  endif ()
endif ()

# XXX(kitware): needed for various headers.
set(H5_STDC_HEADERS 1)
if (NOT WIN32)
  set(H5_TIME_WITH_SYS_TIME 1)
endif ()

#-----------------------------------------------------------------------------
# Generate the H5pubconf.h file containing user settings needed by compilation
#-----------------------------------------------------------------------------
configure_file (${HDF_RESOURCES_DIR}/H5pubconf.h.in ${HDF5_BINARY_DIR}/H5pubconf.h @ONLY)

#-----------------------------------------------------------------------------
# Options for use by cross compiling and toolchains
#-----------------------------------------------------------------------------
option (HDF5_USE_PREGEN "Use pre-generated Files" OFF)
option (HDF5_BATCH_H5DETECT "Use a batch command for running h5detect" OFF)

include (CMakeInstallation.cmake)
vtk_module_install_headers(
  FILES   "${HDF5_BINARY_DIR}/H5pubconf.h"
  SUBDIR  "vtkhdf5")

vtk_module_add_module(VTK::hdf5
  HEADER_ONLY)
target_link_libraries(hdf5
  INTERFACE
    vtkhdf5_src
    vtkhdf5_hl_src)
