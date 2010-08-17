cmake_minimum_required (VERSION 2.8)
PROJECT (HDF5 C CXX)

#-----------------------------------------------------------------------------
# Instructions for use : Normal Build
#
# For standard build of HDF5 libraries,tests and tools.
# Run cmake using the HDF5 source tree to generate a build tree.
# Enable/Disable options according to requirements and
# set CMAKE_INSTALL_PREFIX to the required install path.
# Make install can be used to install all components for system-wide use.
#
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
#   INSTALL (EXPORT target...) syntax), then targets are not configured
#   for export during install.
#
# HDF5_LIB_DEPENDENCIES :
#   If the build of HDF5 libs is being customized, then rules for the
#   dependencies of the HDF5 libs may be 'incomplete', add additional
#   dependencies to this variable so that external projects pick them up
#
# HDF5_EXTERNAL_LIB_PREFIX :
#   If the parent project needs to install hdf libraries, but avoid 
#   name conflicts with system versions, then a prefix may be added
#   to ensure that the correct versions configured are used.
#
# HDF5_INSTALL_BIN_DIR, HDF5_INSTALL_LIB_DIR, HDF5_INSTALL_INCLUDE_DIR :
#   Customize the 'bin', 'lib', and 'include' installation directories.
#
# HDF5_INSTALL_NO_DEVELOPMENT :
#   Set to true to skip installation of headers and CMake package files.
#
# Consider this example from the ParaView project, it builds its own zlib
# library and tells HDF5 to add it as a dependency - this ensures that
# any project making use of this build of HDF5 will use the correct zlib
#
#   # Tell hdf5 that we are manually overriding certain settings
#   SET(HDF5_EXTERNALLY_CONFIGURED 1)
#   # Avoid duplicating names of installed libraries
#   SET(HDF5_EXTERNAL_LIB_PREFIX "vtk")
#   # Export configuration to this export variable
#   SET(HDF5_EXPORTED_TARGETS "paraview-targets")
#
#   # Setup all necessary overrides for zlib so that HDF5 uses our
#   # internally compiled zlib rather than any other version
#   IF(HDF5_ENABLE_Z_LIB_SUPPORT)
#     # We must tell the main HDF5 library that it depends on our zlib 
#     SET(HDF5_LIB_DEPENDENCIES vtkzlib)    
#     # Override the zlib header file
#     IF(VTK_USE_SYSTEM_ZLIB)
#       SET(H5_ZLIB_HEADER "zlib.h")
#     ELSE(VTK_USE_SYSTEM_ZLIB)
#       SET(H5_ZLIB_HEADER "vtk_zlib.h")
#       # Set vars that FindZlib would have set if used in sub project
#       SET(ZLIB_INCLUDE_DIRS "${VTK_ZLIB_INCLUDE_DIRS}")
#       SET(ZLIB_LIBRARIES vtkzlib)
#     ENDIF(VTK_USE_SYSTEM_ZLIB)
#   ENDIF(HDF5_ENABLE_Z_LIB_SUPPORT)
# 
#   # Add the sub project
#   ADD_SUBDIRECTORY(Utilities/hdf5-1.8)
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Set the core names of all the libraries
#-----------------------------------------------------------------------------
SET (HDF5_LIB_CORENAME              "hdf5")
SET (HDF5_TEST_LIB_CORENAME         "hdf5_test")
SET (HDF5_CPP_LIB_CORENAME          "hdf5_cpp")
SET (HDF5_HL_LIB_CORENAME           "hdf5_hl")
SET (HDF5_HL_CPP_LIB_CORENAME       "hdf5_hl_cpp")
SET (HDF5_TOOLS_LIB_CORENAME        "hdf5_tools")
SET (HDF5_F90_LIB_CORENAME          "hdf5_fortran")
SET (HDF5_F90_C_LIB_CORENAME        "hdf5_f90cstub")
SET (HDF5_F90_TEST_LIB_CORENAME     "hdf5_test_fortran")
SET (HDF5_F90_C_TEST_LIB_CORENAME   "hdf5_test_f90cstub")
SET (HDF5_HL_F90_LIB_CORENAME       "hdf5_hl_fortran")
SET (HDF5_HL_F90_C_LIB_CORENAME     "hdf5_hl_f90cstub")

#-----------------------------------------------------------------------------
# Set the true names of all the libraries if customized by external project
#-----------------------------------------------------------------------------
SET (HDF5_LIB_NAME              "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_LIB_CORENAME}")
SET (HDF5_TEST_LIB_NAME         "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_TEST_LIB_CORENAME}")
SET (HDF5_CPP_LIB_NAME          "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_CPP_LIB_CORENAME}")
SET (HDF5_HL_LIB_NAME           "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_HL_LIB_CORENAME}")
SET (HDF5_HL_CPP_LIB_NAME       "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_HL_CPP_LIB_CORENAME}")
SET (HDF5_TOOLS_LIB_NAME        "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_TOOLS_LIB_CORENAME}")
SET (HDF5_F90_LIB_NAME          "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_F90_LIB_CORENAME}")
SET (HDF5_F90_C_LIB_NAME        "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_F90_C_LIB_CORENAME}")
SET (HDF5_F90_TEST_LIB_NAME     "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_F90_TEST_LIB_CORENAME}")
SET (HDF5_F90_C_TEST_LIB_NAME   "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_F90_C_TEST_LIB_CORENAME}")
SET (HDF5_HL_F90_LIB_NAME       "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_HL_F90_LIB_CORENAME}")
SET (HDF5_HL_F90_C_LIB_NAME     "${HDF5_EXTERNAL_LIB_PREFIX}${HDF5_HL_F90_C_LIB_CORENAME}")

#-----------------------------------------------------------------------------
# Set the target names of all the libraries
#-----------------------------------------------------------------------------
SET (HDF5_LIB_TARGET              ${HDF5_LIB_CORENAME})
SET (HDF5_TEST_LIB_TARGET         ${HDF5_TEST_LIB_CORENAME})
SET (HDF5_CPP_LIB_TARGET          ${HDF5_CPP_LIB_CORENAME})
SET (HDF5_HL_LIB_TARGET           ${HDF5_HL_LIB_CORENAME})
SET (HDF5_HL_CPP_LIB_TARGET       ${HDF5_HL_CPP_LIB_CORENAME})
SET (HDF5_TOOLS_LIB_TARGET        ${HDF5_TOOLS_LIB_CORENAME})
SET (HDF5_F90_LIB_TARGET          ${HDF5_F90_LIB_CORENAME})
SET (HDF5_F90_C_LIB_TARGET        ${HDF5_F90_C_LIB_CORENAME})
SET (HDF5_F90_TEST_LIB_TARGET     ${HDF5_F90_TEST_LIB_CORENAME})
SET (HDF5_F90_C_TEST_LIB_TARGET   ${HDF5_F90_C_TEST_LIB_CORENAME})
SET (HDF5_HL_F90_LIB_TARGET       ${HDF5_HL_F90_LIB_CORENAME})
SET (HDF5_HL_F90_C_LIB_TARGET     ${HDF5_HL_F90_C_LIB_CORENAME})

#-----------------------------------------------------------------------------
# Define some CMake variables for use later in the project
#-----------------------------------------------------------------------------
SET (HDF5_RESOURCES_DIR      ${HDF5_SOURCE_DIR}/config/cmake)
SET (HDF5_SRC_DIR            ${HDF5_SOURCE_DIR}/src)
SET (HDF5_TEST_SRC_DIR       ${HDF5_SOURCE_DIR}/test)
SET (HDF5_CPP_SRC_DIR        ${HDF5_SOURCE_DIR}/c++)
SET (HDF5_CPP_TST_DIR        ${HDF5_SOURCE_DIR}/c++/test)
SET (HDF5_HL_SRC_DIR         ${HDF5_SOURCE_DIR}/hl)
SET (HDF5_HL_CPP_SRC_DIR     ${HDF5_SOURCE_DIR}/hl/c++)
SET (HDF5_TOOLS_SRC_DIR      ${HDF5_SOURCE_DIR}/tools)
SET (HDF5_PERFORM_SRC_DIR    ${HDF5_SOURCE_DIR}/perform)
SET (HDF5_F90_SRC_DIR        ${HDF5_SOURCE_DIR}/fortran)

# set default prefix location
SET (CMAKE_INSTALL_PREFIX "./hdf5" CACHE PATH "Install path prefix, prepended onto install directories")

IF (NOT HDF5_INSTALL_BIN_DIR)
  SET (HDF5_INSTALL_BIN_DIR bin)
ENDIF (NOT HDF5_INSTALL_BIN_DIR)
IF (NOT HDF5_INSTALL_LIB_DIR)
  SET (HDF5_INSTALL_LIB_DIR lib)
ENDIF (NOT HDF5_INSTALL_LIB_DIR)
IF (NOT HDF5_INSTALL_INCLUDE_DIR)
  SET (HDF5_INSTALL_INCLUDE_DIR include)
ENDIF (NOT HDF5_INSTALL_INCLUDE_DIR)

#-----------------------------------------------------------------------------
# parse the full version number from H5public.h and include in H5_VERS_INFO
#-----------------------------------------------------------------------------
FILE (READ ${HDF5_SRC_DIR}/H5public.h _h5public_h_contents)
STRING (REGEX REPLACE ".*#define[ \t]+H5_VERS_MAJOR[ \t]+([0-9]*).*$"
    "\\1" H5_VERS_MAJOR ${_h5public_h_contents})
STRING (REGEX REPLACE ".*#define[ \t]+H5_VERS_MINOR[ \t]+([0-9]*).*$"
    "\\1" H5_VERS_MINOR ${_h5public_h_contents})
STRING (REGEX REPLACE ".*#define[ \t]+H5_VERS_RELEASE[ \t]+([0-9]*).*$"
    "\\1" H5_VERS_RELEASE ${_h5public_h_contents})
STRING (REGEX REPLACE ".*#define[ \t]+H5_VERS_SUBRELEASE[ \t]+\"([0-9A-Za-z.]*)\".*$"
    "\\1" H5_VERS_SUBRELEASE ${_h5public_h_contents})
#MESSAGE (STATUS "VERSION: ${H5_VERS_MAJOR}.${H5_VERS_MINOR}.${H5_VERS_RELEASE}-${H5_VERS_SUBRELEASE}")

#-----------------------------------------------------------------------------
# Basic HDF5 stuff here
#-----------------------------------------------------------------------------
SET (HDF5_PACKAGE "hdf5")
SET (HDF5_PACKAGE_NAME "HDF5")
SET (HDF5_PACKAGE_VERSION "${H5_VERS_MAJOR}.${H5_VERS_MINOR}.${H5_VERS_RELEASE}")
SET (HDF5_PACKAGE_VERSION_MAJOR "${H5_VERS_MAJOR}.${H5_VERS_MINOR}")
SET (HDF5_PACKAGE_VERSION_MINOR "${H5_VERS_RELEASE}")
SET (HDF5_PACKAGE_STRING "${HDF5_PACKAGE_NAME} ${HDF5_PACKAGE_VERSION}-${H5_VERS_SUBRELEASE}")
SET (HDF5_PACKAGE_TARNAME "hdf5")
SET (HDF5_PACKAGE_URL "http://www.hdfgroup.org")
SET (HDF5_PACKAGE_BUGREPORT "help@hdfgroup.org")

#-----------------------------------------------------------------------------
# Include some macros for reusable code
#-----------------------------------------------------------------------------
INCLUDE (${HDF5_RESOURCES_DIR}/HDF5Macros.cmake)

#-----------------------------------------------------------------------------
# Setup output Directories
#-----------------------------------------------------------------------------
IF (NOT HDF5_EXTERNALLY_CONFIGURED)
  SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY
      ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables."
  )
  SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY
      ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Libraries"
  )
  SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY
      ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all static libraries."
  )
  SET (CMAKE_Fortran_MODULE_DIRECTORY
      ${PROJECT_BINARY_DIR}/bin/fortran CACHE PATH "Single Directory for all fortran modules."
  )
ELSE (NOT HDF5_EXTERNALLY_CONFIGURED)
  # if we are externally configured, but the project uses old cmake scripts
  # this may not be set and utilities like H5detect will fail
  IF (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
  ENDIF (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
ENDIF (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Targets built within this project are exported at Install time for use
# by other projects using FindHDF5. 
#-----------------------------------------------------------------------------
IF (NOT HDF5_EXPORTED_TARGETS)
  SET (HDF5_EXPORTED_TARGETS "hdf5-targets")
ENDIF (NOT HDF5_EXPORTED_TARGETS)

#-----------------------------------------------------------------------------
# To include a library in the list exported by the project AT BUILD TIME,
# add it to this variable. This is NOT used by Make Install, but for projects
# which include hdf5 as a sub-project within their build tree
#-----------------------------------------------------------------------------
SET_GLOBAL_VARIABLE (HDF5_LIBRARIES_TO_EXPORT "")

#-----------------------------------------------------------------------------
# Run all the CMake configuration tests for our build environment
#-----------------------------------------------------------------------------
INCLUDE (${HDF5_RESOURCES_DIR}/ConfigureChecks.cmake)

#-----------------------------------------------------------------------------
# Option to use legacy naming for windows libs/programs, default is legacy
#-----------------------------------------------------------------------------
IF (WIN32)
  OPTION (H5_LEGACY_NAMING "Use Legacy Names for Libraries and Programs" ON)
ENDIF (WIN32)

#-----------------------------------------------------------------------------
# Option to Build Shared/Static libs, default is static
#-----------------------------------------------------------------------------
OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
SET (LIB_TYPE STATIC)
IF (BUILD_SHARED_LIBS)
  SET (LIB_TYPE SHARED)
  SET (H5_BUILT_AS_DYNAMIC_LIB 1)
  IF (MSVC)
    SET (CMAKE_MFC_FLAG 0)
    FOREACH (flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      IF (${flag_var} MATCHES "/MT")
        STRING (REGEX REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
      ENDIF (${flag_var} MATCHES "/MT")
    ENDFOREACH (flag_var)
  ENDIF (MSVC)
ELSE (BUILD_SHARED_LIBS)
  IF (NOT WIN32)
    # should this be a user setting : Everyone uses it anyway ?
    ADD_DEFINITIONS (-DPIC)
  ENDIF (NOT WIN32)
  IF (MSVC)
    SET (CMAKE_MFC_FLAG 0)
    FOREACH (flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      IF (${flag_var} MATCHES "/MD")
        STRING (REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      ENDIF (${flag_var} MATCHES "/MD")
    ENDFOREACH (flag_var)
  ENDIF (MSVC)
ENDIF (BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# Option to use code coverage
#-----------------------------------------------------------------------------
OPTION (HDF5_ENABLE_COVERAGE "Enable code coverage for Libraries and Programs" OFF)
IF (HDF5_ENABLE_COVERAGE)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    SET (LDFLAGS "${LDFLAGS} -fprofile-arcs -ftest-coverage")   
ENDIF (HDF5_ENABLE_COVERAGE)

#-----------------------------------------------------------------------------
# When building utility executables that generate other (source) files :
# we make use of the following variables defined in the root CMakeLists.
# Certain systems may add /Debug or /Release to output paths
# and we need to call the executable from inside the CMake configuration
#-----------------------------------------------------------------------------
SET (EXE_EXT "")
IF (WIN32)
  SET (EXE_EXT ".exe")
  ADD_DEFINITIONS (-DBIND_TO_CURRENT_VCLIBS_VERSION=1)
  ADD_DEFINITIONS (-D_CRT_SECURE_NO_WARNINGS)
ENDIF (WIN32)

SET (MAKE_SYSTEM)
IF (CMAKE_BUILD_TOOL MATCHES "make")
  SET (MAKE_SYSTEM 1)
ENDIF (CMAKE_BUILD_TOOL MATCHES "make")

SET (CFG_INIT "/${CMAKE_CFG_INTDIR}")
IF (MAKE_SYSTEM)
  SET (CFG_INIT "")
ENDIF (MAKE_SYSTEM)

#-----------------------------------------------------------------------------
# Add some definitions for Debug Builds
#-----------------------------------------------------------------------------
IF (CMAKE_BUILD_TYPE MATCHES Debug)
  ADD_DEFINITIONS (-DDEBUG)
  ADD_DEFINITIONS (
      -DH5Z_DEBUG -DH5V_DEBUG -DH5T_DEBUG
      -DH5S_DEBUG -DH5P_DEBUG -DH5O_DEBUG -DH5MM_DEBUG -DH5MF_DEBUG
      -DH5I_DEBUG -DH5HL_DEBUG -DH5HG_DEBUG -DH5G_DEBUG -DH5F_DEBUG
      -DH5E_DEBUG -DH5D_DEBUG -DH5B_DEBUG -DH5AC_DEBUG -UNDEBUG
  )
  OPTION (HDF5_ENABLE_TRACE "Enable API tracing capability" ON)
  IF (HDF5_ENABLE_TRACE)
    ADD_DEFINITIONS (-DH5_DEBUG_API )
  ENDIF (HDF5_ENABLE_TRACE)
  #-- NMake Makefiles will overwhelm the console with warnings if -Wall is used.
  IF (NOT WIN32)
    ADD_DEFINITIONS (-Wall)
  ENDIF (NOT WIN32)
ELSE (CMAKE_BUILD_TYPE MATCHES Debug)
  ADD_DEFINITIONS (-UH5_DEBUG_API -DNDEBUG)
  OPTION (HDF5_ENABLE_TRACE "Enable API tracing capability" OFF)
  IF (HDF5_ENABLE_TRACE)
    ADD_DEFINITIONS (-DH5_DEBUG_API )
  ENDIF (HDF5_ENABLE_TRACE)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

#-----------------------------------------------------------------------------
# Compiler specific flags : Shouldn't there be compiler tests for these
#-----------------------------------------------------------------------------
IF (CMAKE_COMPILER_IS_GNUCC)
  SET (CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS} -std=c99 -fomit-frame-pointer -finline-functions -fno-common")
ENDIF (CMAKE_COMPILER_IS_GNUCC)

#-----------------------------------------------------------------------------
# Option to allow the user to disable compiler warnings
#-----------------------------------------------------------------------------
OPTION (HDF5_DISABLE_COMPILER_WARNINGS "Disable compiler warnings" OFF)
IF (HDF5_DISABLE_COMPILER_WARNINGS)
  # MSVC uses /w to suppress warnings.  It also complains if another
  # warning level is given, so remove it.
  IF (MSVC)
    SET (HDF5_WARNINGS_BLOCKED 1)
    STRING (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " "
        CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w")
  ENDIF (MSVC)
  IF (WIN32)
    ADD_DEFINITIONS (-D_CRT_SECURE_NO_WARNINGS)
  ENDIF (WIN32)
  # Borland uses -w- to suppress warnings.
  IF (BORLAND)
    SET (HDF5_WARNINGS_BLOCKED 1)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w-")
  ENDIF (BORLAND)

  # Most compilers use -w to suppress warnings.
  IF (NOT HDF5_WARNINGS_BLOCKED)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
  ENDIF (NOT HDF5_WARNINGS_BLOCKED)
ENDIF (HDF5_DISABLE_COMPILER_WARNINGS)

#-----------------------------------------------------------------------------
# This is in here to help some of the GCC based IDES like Eclipse
# and code blocks parse the compiler errors and warnings better.
#-----------------------------------------------------------------------------
IF (CMAKE_COMPILER_IS_GNUCC)
  SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
ENDIF (CMAKE_COMPILER_IS_GNUCC)
IF (CMAKE_COMPILER_IS_GNUCXX)
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
ENDIF (CMAKE_COMPILER_IS_GNUCXX)

#-----------------------------------------------------------------------------
# All libs/tests/examples need the main include directories
#-----------------------------------------------------------------------------
INCLUDE_DIRECTORIES (${HDF5_BINARY_DIR} ${HDF5_SRC_DIR})

#-----------------------------------------------------------------------------
# Option to Enable MPI Parallel
#-----------------------------------------------------------------------------
SET (CMAKE_MODULE_PATH ${HDF5_RESOURCES_DIR} ${CMAKE_MODULE_PATH})
OPTION (HDF5_ENABLE_PARALLEL "Enable parallel build (requires MPI)" OFF)
IF (HDF5_ENABLE_PARALLEL)
  INCLUDE (FindMPI)
  INCLUDE_DIRECTORIES (${MPI_INCLUDE_PATH})
  IF (MPI_FOUND)
    SET (H5_HAVE_PARALLEL 1)
    # MPI checks, only do these if MPI_FOUND is true, otherwise they always fail
    # and once set, they are cached as false and not regenerated
    SET (CMAKE_REQUIRED_INCLUDES "${MPI_INCLUDE_PATH}/mpi.h" )
    SET (CMAKE_REQUIRED_LIBRARIES "${MPI_LIBRARY}" )
    CHECK_FUNCTION_EXISTS (MPI_File_get_size H5_HAVE_MPI_GET_SIZE) 
    # Used by Fortran + MPI
    CHECK_SYMBOL_EXISTS (MPI_Comm_c2f "${MPI_INCLUDE_PATH}/mpi.h"  H5_HAVE_MPI_MULTI_LANG_Comm)
    CHECK_SYMBOL_EXISTS (MPI_Info_c2f "${MPI_INCLUDE_PATH}/mpi.h"  H5_HAVE_MPI_MULTI_LANG_Info)
  ENDIF (MPI_FOUND)
ENDIF (HDF5_ENABLE_PARALLEL)

# Parallel IO usage requires MPI to be Linked and Included
IF (H5_HAVE_PARALLEL)
  SET (LINK_LIBS ${LINK_LIBS} ${MPI_LIBRARY})
  IF (MPI_LINK_FLAGS)
    SET (CMAKE_EXE_LINKER_FLAGS ${MPI_LINK_FLAGS} ${CMAKE_EXE_LINKER_FLAGS})
  ENDIF (MPI_LINK_FLAGS)
  IF (MPI_EXTRA_LIBRARY)
    SET (LINK_LIBS ${LINK_LIBS} ${MPI_EXTRA_LIBRARY})
  ENDIF (MPI_EXTRA_LIBRARY)
ENDIF (H5_HAVE_PARALLEL)

#-----------------------------------------------------------------------------
# Option to use 1.6.x API
#-----------------------------------------------------------------------------
OPTION (HDF5_USE_16_API_DEFAULT "Use the HDF5 1.6.x API by default" OFF)
SET (H5_USE_16_API_DEFAULT 0)
IF (HDF5_USE_16_API_DEFAULT)
  SET (H5_USE_16_API_DEFAULT 1)
ENDIF (HDF5_USE_16_API_DEFAULT)

#-----------------------------------------------------------------------------
# Options for HDF5 Filters
#-----------------------------------------------------------------------------
MACRO (HDF5_SETUP_FILTERS FILTER)
  OPTION (HDF5_USE_FILTER_${FILTER} "Use the ${FILTER} Filter" ON)
  IF (HDF5_USE_FILTER_${FILTER})
    SET (H5_HAVE_FILTER_${FILTER} 1)
  ENDIF (HDF5_USE_FILTER_${FILTER})
  # MESSAGE (STATUS "Filter ${FILTER} is ${HDF5_USE_FILTER_${FILTER}}")
ENDMACRO (HDF5_SETUP_FILTERS)

HDF5_SETUP_FILTERS (SHUFFLE)
HDF5_SETUP_FILTERS (FLETCHER32)
HDF5_SETUP_FILTERS (NBIT)
HDF5_SETUP_FILTERS (SCALEOFFSET)

#-----------------------------------------------------------------------------
# Option for ZLib support
#-----------------------------------------------------------------------------
OPTION (HDF5_ENABLE_Z_LIB_SUPPORT "Enable Zlib Filters" OFF)
IF (HDF5_ENABLE_Z_LIB_SUPPORT)
  IF (NOT H5_ZLIB_HEADER)
    FIND_PACKAGE (ZLIB REQUIRED)
    IF (ZLIB_FOUND)
      SET (H5_HAVE_FILTER_DEFLATE 1)
      SET (H5_HAVE_ZLIB_H 1)
      SET (H5_HAVE_LIBZ 1)
      SET (H5_ZLIB_HEADER "zlib.h")
    ELSE (ZLIB_FOUND)
      MESSAGE (FATAL " ZLib is Required for ZLib support in HDF5")
    ENDIF (ZLIB_FOUND)
  ELSE (NOT H5_ZLIB_HEADER)
    # This project is being called from within another and ZLib is already configured
    SET (H5_HAVE_FILTER_DEFLATE 1)
    SET (H5_HAVE_ZLIB_H 1)
    SET (H5_HAVE_LIBZ 1)
  ENDIF (NOT H5_ZLIB_HEADER)
  SET (LINK_LIBS ${LINK_LIBS} ${ZLIB_LIBRARIES})
  INCLUDE_DIRECTORIES (${ZLIB_INCLUDE_DIRS})
  MESSAGE (STATUS "Filter ZLIB is ON")
ENDIF (HDF5_ENABLE_Z_LIB_SUPPORT)

#-----------------------------------------------------------------------------
# Option for SzLib support
#-----------------------------------------------------------------------------
SET (CMAKE_MODULE_PATH ${HDF5_RESOURCES_DIR} ${CMAKE_MODULE_PATH})
OPTION (HDF5_ENABLE_SZIP_SUPPORT "Use SZip Filter" OFF)
IF (HDF5_ENABLE_SZIP_SUPPORT)
  FIND_PACKAGE (SZIP REQUIRED)
  IF (SZIP_FOUND)
    SET (H5_HAVE_FILTER_SZIP 1)
    SET (H5_HAVE_SZLIB_H 1)
    SET (H5_HAVE_LIBSZ 1)
    SET (LINK_LIBS ${LINK_LIBS} ${SZIP_LIBRARIES})
    INCLUDE_DIRECTORIES (${SZIP_INCLUDE_DIRS})
#      MESSAGE (STATUS "SZIP_INCLUDE_DIRS: ${SZIP_INCLUDE_DIRS}")
#      MESSAGE (STATUS "SZIP_INCLUDE_DIR: ${SZIP_INCLUDE_DIR}")
    MESSAGE (STATUS "Filter SZIP is ON")
    OPTION (HDF5_ENABLE_SZIP_ENCODING "Use SZip Encoding" OFF)
    IF (HDF5_ENABLE_SZIP_ENCODING)
      SET (H5_HAVE_SZIP_ENCODER 1)
    ENDIF (HDF5_ENABLE_SZIP_ENCODING)
  ELSE (SZIP_FOUND)
      MESSAGE (FATAL_ERROR "SZIP is Required for SZIP support in HDF5")
  ENDIF (SZIP_FOUND)
ENDIF (HDF5_ENABLE_SZIP_SUPPORT)

#-----------------------------------------------------------------------------
# Option for external libraries on windows
#-----------------------------------------------------------------------------
IF (WIN32)
  OPTION (HDF5_PACKAGE_EXTLIBS "CPACK - include external libraries" OFF)
  IF (BUILD_SHARED_LIBS)
    FILE (MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BLDTYPE})
    
    IF (HDF5_ENABLE_Z_LIB_SUPPORT AND ZLIB_FOUND)
      GET_FILENAME_COMPONENT(ZLIB_DLL_NAME ${ZLIB_LIBRARY} NAME_WE)
      # MESSAGE (STATUS "ZLIB_DLL_NAME: ${ZLIB_DLL_NAME}")
      GET_FILENAME_COMPONENT(ZLIB_BIN_PATH ${ZLIB_LIBRARY} PATH)
      # MESSAGE (STATUS "ZLIB_BIN_PATH: ${ZLIB_BIN_PATH}")
      ADD_CUSTOM_TARGET (ZLIB-Release-Copy ALL
          COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ZLIB_BIN_PATH}/${ZLIB_DLL_NAME}.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/
          COMMENT "Copying ${ZLIB_BIN_PATH}/${ZLIB_DLL_NAME}.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/"
      )
    ENDIF (HDF5_ENABLE_Z_LIB_SUPPORT AND ZLIB_FOUND)
    
    IF (HDF5_ENABLE_SZIP_SUPPORT AND SZIP_FOUND)
      GET_FILENAME_COMPONENT(SZIP_DLL_NAME ${SZIP_LIBRARY} NAME_WE)
      # MESSAGE (STATUS "SZIP_DLL_NAME: ${SZIP_DLL_NAME}")
      GET_FILENAME_COMPONENT(SZIP_BIN_PATH ${SZIP_LIBRARY} PATH)
      # MESSAGE (STATUS "SZIP_BIN_PATH: ${SZIP_BIN_PATH}")
      ADD_CUSTOM_TARGET (SZIP-Release-Copy ALL
          COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SZIP_BIN_PATH}/${SZIP_DLL_NAME}.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/
          COMMENT "Copying ${SZIP_BIN_PATH}/${SZIP_DLL_NAME}.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/"
      )
    ENDIF (HDF5_ENABLE_SZIP_SUPPORT AND SZIP_FOUND)
    
  ENDIF (BUILD_SHARED_LIBS)
ENDIF (WIN32)

#-----------------------------------------------------------------------------
# Option to use threadsafe
# Note: Currently CMake only allows configuring of threadsafe on WINDOWS.
#-----------------------------------------------------------------------------
IF (WIN32)
  OPTION (HDF5_ENABLE_THREADSAFE "Enable Threadsafety" OFF)
  IF (HDF5_ENABLE_THREADSAFE)
    SET (H5_HAVE_WIN_THREADS 1)
    SET (H5_HAVE_THREADSAFE 1)
  ENDIF (HDF5_ENABLE_THREADSAFE)
ENDIF (WIN32)

#-----------------------------------------------------------------------------
# Option to use PACKED BITS SUPPORT
#-----------------------------------------------------------------------------
OPTION (HDF5_USE_H5DUMP_PACKED_BITS "Use the PACKED BITS feature in h5dump" OFF)
SET (H5_HAVE_H5DUMP_PACKED_BITS 0)
IF (HDF5_USE_H5DUMP_PACKED_BITS)
  SET (H5_HAVE_H5DUMP_PACKED_BITS 1)
ENDIF (HDF5_USE_H5DUMP_PACKED_BITS)

#-----------------------------------------------------------------------------
# Add the HDF5 Library Target to the build
#-----------------------------------------------------------------------------
ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src)

#-----------------------------------------------------------------------------
# Build utility to copy and strip X lines of file
#-----------------------------------------------------------------------------
IF (HDF5_BUILD_TOOLS AND BUILD_TESTING)
  SET (XLATE_UTILITY "xlatefile")
  ADD_EXECUTABLE(${XLATE_UTILITY} ${HDF5_RESOURCES_DIR}/xlatefile.c)
ENDIF (HDF5_BUILD_TOOLS AND BUILD_TESTING)

#-----------------------------------------------------------------------------
# Dashboard and Testing Settings
#-----------------------------------------------------------------------------
OPTION (BUILD_TESTING "Build HDF5 Unit Testing" OFF)
IF (BUILD_TESTING)
  SET (DART_TESTING_TIMEOUT 1200
      CACHE INTEGER
      "Timeout in seconds for each test (default 1200=20minutes)"
  )
  ENABLE_TESTING ()
  INCLUDE (CTest)
  IF (NOT HDF5_EXTERNALLY_CONFIGURED)
    IF (EXISTS "${HDF5_SOURCE_DIR}/test" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/test")
      ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/tools/lib ${PROJECT_BINARY_DIR}/tools/lib)
      ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/test      ${PROJECT_BINARY_DIR}/test)
    ENDIF (EXISTS "${HDF5_SOURCE_DIR}/test" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/test")
    IF (EXISTS "${HDF5_SOURCE_DIR}/perform" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/perform")
      ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/perform   ${PROJECT_BINARY_DIR}/perform)
    ENDIF(EXISTS "${HDF5_SOURCE_DIR}/perform" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/perform")
    IF (H5_HAVE_PARALLEL)
      IF (EXISTS "${HDF5_SOURCE_DIR}/testpar" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/testpar")
        ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/testpar ${PROJECT_BINARY_DIR}/testpar)
      ENDIF (EXISTS "${HDF5_SOURCE_DIR}/testpar" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/testpar")
    ENDIF (H5_HAVE_PARALLEL)
  ENDIF (NOT HDF5_EXTERNALLY_CONFIGURED)
  INCLUDE (${HDF5_SOURCE_DIR}/CTestConfig.cmake)
ENDIF (BUILD_TESTING)

#-----------------------------------------------------------------------------
# Option to build Fortran bindings/tests/examples
# Make sure this appears before the CONFIGURE_FILE step
# so that fortran name mangling is detected before writing H5pubconf.h
#-----------------------------------------------------------------------------
# Set default name mangling : overridden by Fortran detection in fortran dir
SET (H5_FC_FUNC  "H5_FC_FUNC(name,NAME) name ## _")
SET (H5_FC_FUNC_ "H5_FC_FUNC_(name,NAME) name ## _")
IF (EXISTS "${HDF5_SOURCE_DIR}/fortran" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/fortran")
  OPTION (HDF5_BUILD_FORTRAN "Build FORTRAN support" OFF)
  IF (HDF5_BUILD_FORTRAN)
    ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/fortran ${PROJECT_BINARY_DIR}/fortran)
    IF (HDF5_BUILD_HL_LIB)
      IF (EXISTS "${HDF5_SOURCE_DIR}/hl/fortran" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/hl/fortran")
        #-- Build the High Level Fortran source codes
        ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/hl/fortran ${HDF5_BINARY_DIR}/hl/fortran)
      ENDIF (EXISTS "${HDF5_SOURCE_DIR}/hl/fortran" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/hl/fortran")
    ENDIF (HDF5_BUILD_HL_LIB)
  ENDIF (HDF5_BUILD_FORTRAN)
ENDIF (EXISTS "${HDF5_SOURCE_DIR}/fortran" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/fortran")

#-----------------------------------------------------------------------------
# Option to build examples
#-----------------------------------------------------------------------------
IF (EXISTS "${HDF5_SOURCE_DIR}/examples" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/examples")
  OPTION (HDF5_BUILD_EXAMPLES  "Build HDF5 Library Examples" OFF)
  IF (HDF5_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/examples ${PROJECT_BINARY_DIR}/examples)
  ENDIF (HDF5_BUILD_EXAMPLES)
ENDIF (EXISTS "${HDF5_SOURCE_DIR}/examples" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/examples")

#-----------------------------------------------------------------------------
# Option to build HDF5 C++ Library
#-----------------------------------------------------------------------------
IF (EXISTS "${HDF5_SOURCE_DIR}/c++" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/c++")
  OPTION (HDF5_BUILD_CPP_LIB  "Build HDF5 C++ Library" OFF)
  IF (HDF5_BUILD_CPP_LIB)
    ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/c++ ${PROJECT_BINARY_DIR}/c++)
  ENDIF (HDF5_BUILD_CPP_LIB)
ENDIF (EXISTS "${HDF5_SOURCE_DIR}/c++" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/c++")

#-----------------------------------------------------------------------------
# Option to build HDF5 Tools
#-----------------------------------------------------------------------------
IF (EXISTS "${HDF5_SOURCE_DIR}/tools" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/tools")
  OPTION (HDF5_BUILD_TOOLS  "Build HDF5 Tools" OFF)
  IF (HDF5_BUILD_TOOLS)
    ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/tools ${PROJECT_BINARY_DIR}/tools)
  ENDIF (HDF5_BUILD_TOOLS)
ENDIF (EXISTS "${HDF5_SOURCE_DIR}/tools" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/tools")

#-----------------------------------------------------------------------------
# Option to build High Level API's
#-----------------------------------------------------------------------------
IF (EXISTS "${HDF5_SOURCE_DIR}/hl" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/hl")
  OPTION (HDF5_BUILD_HL_LIB  "Build HIGH Level HDF5 Library" OFF)
  IF (HDF5_BUILD_HL_LIB)
    SET (H5_INCLUDE_HL 1)
    ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/hl ${PROJECT_BINARY_DIR}/hl)
  ENDIF (HDF5_BUILD_HL_LIB)
ENDIF (EXISTS "${HDF5_SOURCE_DIR}/hl" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/hl")

#-----------------------------------------------------------------------------
# Generate the H5pubconf.h file containing user settings needed by compilation
#-----------------------------------------------------------------------------
CONFIGURE_FILE (${HDF5_RESOURCES_DIR}/H5pubconf.h.in ${HDF5_BINARY_DIR}/H5pubconf.h @ONLY)

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
IF (NOT HDF5_INSTALL_NO_DEVELOPMENT)
  INSTALL (
      FILES ${PROJECT_BINARY_DIR}/H5pubconf.h
      DESTINATION ${HDF5_INSTALL_INCLUDE_DIR}
      COMPONENT headers
  )
ENDIF (NOT HDF5_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Option for external libraries on windows
#-----------------------------------------------------------------------------
IF (NOT HDF5_EXTERNALLY_CONFIGURED)
  IF (HDF5_PACKAGE_EXTLIBS)
    IF (HDF5_ENABLE_Z_LIB_SUPPORT AND ZLIB_FOUND)
      INSTALL (
          FILES ${ZLIB_INCLUDE_DIR}/zconf.h ${ZLIB_INCLUDE_DIR}/zlib.h
          DESTINATION ${HDF5_INSTALL_INCLUDE_DIR}
          COMPONENT headers
      )
      INSTALL(
          FILES ${ZLIB_LIBRARY}
          DESTINATION ${HDF5_INSTALL_LIB_DIR}
          COMPONENT libraries
      )
      IF (BUILD_SHARED_LIBS)
        GET_FILENAME_COMPONENT(ZLIB_DLL_NAME ${ZLIB_LIBRARY} NAME_WE)
        # message(STATUS "ZLIB_DLL_NAME: ${ZLIB_DLL_NAME}")
        GET_FILENAME_COMPONENT(ZLIB_BIN_PATH ${ZLIB_LIBRARY} PATH)
        # message(STATUS "ZLIB_BIN_PATH: ${ZLIB_BIN_PATH}")
        INSTALL(
            FILES ${ZLIB_BIN_PATH}/${ZLIB_DLL_NAME}.dll
            DESTINATION ${HDF5_INSTALL_BIN_DIR}
            COMPONENT libraries
        )
      ENDIF (BUILD_SHARED_LIBS)
    ENDIF (HDF5_ENABLE_Z_LIB_SUPPORT AND ZLIB_FOUND)
    IF (HDF5_ENABLE_SZIP_SUPPORT AND SZIP_FOUND)
      INSTALL (
          FILES ${SZIP_INCLUDE_DIR}/SZconfig.h ${SZIP_INCLUDE_DIR}/szlib.h ${SZIP_INCLUDE_DIR}/szip_adpt.h ${SZIP_INCLUDE_DIR}/ricehdf.h
          DESTINATION ${HDF5_INSTALL_INCLUDE_DIR}
          COMPONENT headers
      )
      INSTALL(
          FILES ${SZIP_LIBRARY}
          DESTINATION ${HDF5_INSTALL_LIB_DIR}
          COMPONENT libraries
      )
      IF (BUILD_SHARED_LIBS)
        GET_FILENAME_COMPONENT(SZIP_DLL_NAME ${SZIP_LIBRARY} NAME_WE)
        # message(STATUS "SZIP_DLL_NAME: ${SZIP_DLL_NAME}")
        GET_FILENAME_COMPONENT(SZIP_BIN_PATH ${SZIP_LIBRARY} PATH)
        # message(STATUS "SZIP_BIN_PATH: ${ZLIB_BIN_PATH}")
        INSTALL(
            FILES ${SZIP_BIN_PATH}/${SZIP_DLL_NAME}.dll
            DESTINATION ${HDF5_INSTALL_BIN_DIR}
            COMPONENT libraries
        )
      ENDIF (BUILD_SHARED_LIBS)
    ENDIF (HDF5_ENABLE_SZIP_SUPPORT AND SZIP_FOUND)
  ENDIF (HDF5_PACKAGE_EXTLIBS)
ENDIF (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
IF (NOT HDF5_EXTERNALLY_CONFIGURED)
  INSTALL (
      EXPORT ${HDF5_EXPORTED_TARGETS}
      DESTINATION ${HDF5_INSTALL_LIB_DIR}/cmake/hdf5-${HDF5_PACKAGE_VERSION}
      FILE hdf5-targets.cmake
  )
ENDIF (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Export all exported targets to the build tree for use by parent project
#-----------------------------------------------------------------------------
IF (NOT HDF5_EXTERNALLY_CONFIGURED)
  EXPORT (
      TARGETS ${HDF5_LIBRARIES_TO_EXPORT} ${HDF5_LIB_DEPENDENCIES}
      FILE hdf5-targets.cmake
  )
ENDIF (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Configure the hdf5-config.cmake file for the build directory
#-----------------------------------------------------------------------------
SET (HDF5_INCLUDES_BUILD_TIME
    ${HDF5_SRC_DIR} ${HDF5_CPP_SRC_DIR} ${HDF5_HL_SRC_DIR}
    ${HDF5_TOOLS_SRC_DIR} ${HDF5_BINARY_DIR}
)
SET (HDF5_VERSION_STRING @HDF5_PACKAGE_VERSION@)
SET (HDF5_VERSION_MAJOR  @HDF5_PACKAGE_VERSION_MAJOR@)
SET (HDF5_VERSION_MINOR  @HDF5_PACKAGE_VERSION_MINOR@)

CONFIGURE_FILE (
    ${HDF5_RESOURCES_DIR}/hdf5-config.cmake.build.in 
    ${HDF5_BINARY_DIR}/hdf5-config.cmake @ONLY
)

#-----------------------------------------------------------------------------
# Configure the hdf5-config.cmake file for the install directory
#-----------------------------------------------------------------------------
IF (NOT HDF5_EXTERNALLY_CONFIGURED)
  CONFIGURE_FILE (
      ${HDF5_RESOURCES_DIR}/hdf5-config.cmake.install.in
      ${HDF5_BINARY_DIR}/CMakeFiles/hdf5-config.cmake @ONLY
  )
  INSTALL (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/hdf5-config.cmake
      DESTINATION ${HDF5_INSTALL_LIB_DIR}/cmake/hdf5-${HDF5_PACKAGE_VERSION}
  )
ENDIF (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Configure the hdf5-config-version .cmake file for the install directory
#-----------------------------------------------------------------------------
IF (NOT HDF5_EXTERNALLY_CONFIGURED)
  CONFIGURE_FILE (
      ${HDF5_RESOURCES_DIR}/hdf5-config-version.cmake.in
      ${HDF5_BINARY_DIR}/CMakeFiles/hdf5-config-version.cmake @ONLY
  )
  INSTALL (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/hdf5-config-version.cmake
      DESTINATION ${HDF5_INSTALL_LIB_DIR}/cmake/hdf5-${HDF5_PACKAGE_VERSION}
  )
ENDIF (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Set the cpack variables
#-----------------------------------------------------------------------------
IF (NOT HDF5_EXTERNALLY_CONFIGURED AND EXISTS "${HDF5_SOURCE_DIR}/release_docs")
  SET (CPACK_PACKAGE_VENDOR "The HDF Group")
  SET (CPACK_PACKAGE_NAME "${HDF5_PACKAGE_NAME}")
  SET (CPACK_PACKAGE_VERSION "${HDF5_PACKAGE_VERSION}")
  SET (CPACK_PACKAGE_VERSION_MAJOR "${HDF5_PACKAGE_VERSION_MAJOR}")
  SET (CPACK_PACKAGE_VERSION_MINOR "${HDF5_PACKAGE_VERSION_MINOR}")
  SET (CPACK_PACKAGE_VERSION_PATCH "")
  SET (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/release_docs/COPYING")
  
  IF (WIN32)
    SET (CPACK_NSIS_PACKAGE_NAME "HDF5 ${HDF5_PACKAGE_VERSION}")
  ENDIF (WIN32)
  
  INCLUDE (CPack)

  #---------------------------------------------------------------------------
  # Now list the cpack commands
  #---------------------------------------------------------------------------
  CPACK_ADD_COMPONENT (hdfapplications 
      DISPLAY_NAME "HDF5 Applications" 
      DEPENDS libraries
      GROUP Applications
  )
  CPACK_ADD_COMPONENT (libraries 
      DISPLAY_NAME "HDF5 Libraries"
      GROUP Runtime
  )
  CPACK_ADD_COMPONENT (headers 
      DISPLAY_NAME "HDF5 Headers" 
      DEPENDS libraries
      GROUP Development
  )
  
  IF (HDF5_BUILD_FORTRAN)
    CPACK_ADD_COMPONENT (fortlibraries 
        DISPLAY_NAME "HDF5 Fortran Libraries" 
        DEPENDS libraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (fortheaders 
        DISPLAY_NAME "HDF5 Fortran Headers" 
        DEPENDS fortlibraries
        GROUP Development
    )
  ENDIF (HDF5_BUILD_FORTRAN)
  
  IF (HDF5_BUILD_CPP_LIB)
    CPACK_ADD_COMPONENT (cpplibraries 
        DISPLAY_NAME "HDF5 C++ Libraries" 
        DEPENDS libraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (cppheaders 
        DISPLAY_NAME "HDF5 C++ Headers" 
        DEPENDS cpplibraries
        GROUP Development
    )
  ENDIF (HDF5_BUILD_CPP_LIB)
  
  IF (HDF5_BUILD_TOOLS)
    CPACK_ADD_COMPONENT (toolsapplications 
        DISPLAY_NAME "HDF5 Tools Applications" 
        DEPENDS toolslibraries
        GROUP Applications
    )
    CPACK_ADD_COMPONENT (toolslibraries 
        DISPLAY_NAME "HDF5 Tools Libraries" 
        DEPENDS libraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (toolsheaders 
        DISPLAY_NAME "HDF5 Tools Headers" 
        DEPENDS toolslibraries
        GROUP Development
    )
  ENDIF (HDF5_BUILD_TOOLS)
  
  IF (HDF5_BUILD_HL_LIB)
    CPACK_ADD_COMPONENT (hllibraries 
        DISPLAY_NAME "HDF5 HL Libraries" 
        DEPENDS libraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (hlheaders 
        DISPLAY_NAME "HDF5 HL Headers" 
        DEPENDS hllibraries
        GROUP Development
    )
    CPACK_ADD_COMPONENT (hltoolsapplications 
        DISPLAY_NAME "HDF5 HL Tools Applications" 
        DEPENDS hllibraries
        GROUP Applications
    )
    CPACK_ADD_COMPONENT (hlcpplibraries 
        DISPLAY_NAME "HDF5 HL C++ Libraries" 
        DEPENDS hllibraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (hlcppheaders 
        DISPLAY_NAME "HDF5 HL C++ Headers" 
        DEPENDS hlcpplibraries
        GROUP Development
    )
    CPACK_ADD_COMPONENT (hlfortlibraries 
        DISPLAY_NAME "HDF5 HL Fortran Libraries" 
        DEPENDS fortlibraries
        GROUP Runtime
    )
  ENDIF (HDF5_BUILD_HL_LIB)
  
ENDIF (NOT HDF5_EXTERNALLY_CONFIGURED AND EXISTS "${HDF5_SOURCE_DIR}/release_docs")
