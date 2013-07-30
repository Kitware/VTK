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
# Consider this example from the ParaView project, it builds its own zlib
# library and tells HDF5 to add it as a dependency - this ensures that
# any project making use of this build of HDF5 will use the correct zlib
#
#   # Tell hdf5 that we are manually overriding certain settings
#   SET(HDF5_EXTERNALLY_CONFIGURED 1)
#   # Avoid duplicating names of installed libraries
#   SET(HDF5_EXTERNAL_LIB_PREFIX "vtk")
#   # Targets linking to the HDF5 libs need to know their names
#   # if they are changed in the sub project, they should be here too
#   SET(HDF5_LIB_NAME "vtkhdf5")
#   SET(HDF5_HL_LIB_NAME "vtkhdf5_hl")
#   SET(HDF5_LIBRARY "${HDF5_LIB_NAME};${HDF5_HL_LIB_NAME}")
#   # Export configuration to this export variable
#   SET(HDF5_EXPORTED_TARGETS "paraview-targets")
#
#   # Setup all necessary overrides for zlib so that HDF5 uses our
#   # internally compiled zlib rather than any other version
#   IF(HDF5_ENABLE_Z_LIB_SUPPORT)
#    # We must tell the main HDF5 library that it depends on our zlib 
#     SET(HDF5_LIB_DEPENDENCIES vtkzlib)    
#     # Override the zlib header file
#     IF(VTK_USE_SYSTEM_ZLIB)
#       SET(H5_ZLIB_HEADER "zlib.h")
#     ELSE(VTK_USE_SYSTEM_ZLIB)
#      SET(H5_ZLIB_HEADER "vtk_zlib.h")
#       # Set vars that FindZlib would have set if used in sub project
#       SET(ZLIB_INCLUDE_DIRS "${VTK_ZLIB_INCLUDE_DIRS}")
#       SET(ZLIB_LIBRARIES vtkzlib)
#     ENDIF(VTK_USE_SYSTEM_ZLIB)
#  ENDIF(HDF5_ENABLE_Z_LIB_SUPPORT)
#   
#   # Add the sub project
#   ADD_SUBDIRECTORY(Utilities/hdf5-1.8)
#   # Add the HDf5 dirs to our include path
#   SET(HDF5_INCLUDE_DIR 
#     ${ParaView_SOURCE_DIR}/Utilities/hdf5-1.8/src
#     ${ParaView_BINARY_DIR}/Utilities/hdf5-1.8
#     ${ParaView_SOURCE_DIR}/Utilities/hdf5-1.8/hl/src)
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Basic HDF5 stuff here
#-----------------------------------------------------------------------------
SET (HDF5_PACKAGE "hdf5")
SET (HDF5_PACKAGE_NAME "HDF5")
SET (HDF5_PACKAGE_VERSION 1.8.5)
SET (HDF5_PACKAGE_VERSION_MAJOR 1.8)
SET (HDF5_PACKAGE_STRING "${HDF5_PACKAGE_NAME} ${HDF5_PACKAGE_VERSION}")
SET (HDF5_PACKAGE_TARNAME "hdf5")
SET (HDF5_PACKAGE_URL "htt://www.hdfgroup.org")
SET (HDF5_PACKAGE_BUGREPORT "help@hdfgroup.org")

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
# Define some CMake variables for use later in the project
#-----------------------------------------------------------------------------
SET (HDF5_RESOURCES_DIR      ${HDF5_SOURCE_DIR}/Resources)
SET (HDF5_SRC_DIR            ${HDF5_SOURCE_DIR}/src)
SET (HDF5_TEST_SOURCE_DIR    ${HDF5_SOURCE_DIR}/test)
SET (HDF5_CPP_SOURCE_DIR     ${HDF5_SOURCE_DIR}/c++)
SET (HDF5_CPP_TEST_DIR       ${HDF5_CPP_SOURCE_DIR}/test)
SET (HDF5_HL_SOURCE_DIR      ${HDF5_SOURCE_DIR}/hl)
SET (HDF5_TOOLS_SOURCE_DIR   ${HDF5_SOURCE_DIR}/tools)
SET (HDF5_PERFORM_SOURCE_DIR ${HDF5_SOURCE_DIR}/perform)
SET (HDF5_F90_SOURCE_DIR     ${HDF5_SOURCE_DIR}/fortran)

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
ENDIF(NOT HDF5_EXTERNALLY_CONFIGURED)

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
SET_GLOBAL_VARIABLE(HDF5_LIBRARIES_TO_EXPORT "")

#-----------------------------------------------------------------------------
# Run all the CMake configuration tests for our build environment
#-----------------------------------------------------------------------------
INCLUDE (${HDF5_SOURCE_DIR}/ConfigureChecks.cmake)

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
  SET (LIBPOSTFIX "")
  IF (MSVC)
    SET (CMAKE_MFC_FLAG 0)
    # flag replace /MT -> /MD disabled in this reduced distribution
  ENDIF (MSVC)
ELSE (BUILD_SHARED_LIBS)
  IF (NOT WIN32)
    # should this be a user setting : Everyone uses it anyway ?
    ADD_DEFINITIONS(-DPIC)
  ENDIF (NOT WIN32)
  SET (LIBPOSTFIX "")
  IF (MSVC)
    SET (CMAKE_MFC_FLAG 0)
    # flag replace /MD -> /MT disabled in this reduced distribution
  ENDIF (MSVC)
ENDIF (BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# Set the target names of all the libraries
#-----------------------------------------------------------------------------
SET (HDF5_LIB_TARGET              ${HDF5_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_TEST_LIB_TARGET         ${HDF5_TEST_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_CPP_LIB_TARGET          ${HDF5_CPP_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_HL_LIB_TARGET           ${HDF5_HL_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_HL_CPP_LIB_TARGET       ${HDF5_HL_CPP_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_TOOLS_LIB_TARGET        ${HDF5_TOOLS_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_F90_LIB_TARGET          ${HDF5_F90_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_F90_C_LIB_TARGET        ${HDF5_F90_C_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_F90_TEST_LIB_TARGET     ${HDF5_F90_TEST_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_F90_C_TEST_LIB_TARGET   ${HDF5_F90_C_TEST_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_HL_F90_LIB_TARGET       ${HDF5_HL_F90_LIB_NAME}${LIBPOSTFIX})
SET (HDF5_HL_F90_C_LIB_TARGET     ${HDF5_HL_F90_C_LIB_NAME}${LIBPOSTFIX})

#-----------------------------------------------------------------------------
# Include the main src and config directories
#-----------------------------------------------------------------------------
SET (HDF5_INCLUDE_DIRECTORIES
    ${HDF5_SRC_DIR}
    ${HDF5_SOURCE_DIR}
    ${HDF5_BINARY_DIR}
)
INCLUDE_DIRECTORIES (${HDF5_INCLUDE_DIRECTORIES})

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
      -DH5_DEBUG_API -DH5Z_DEBUG -DH5V_DEBUG -DH5T_DEBUG
      -DH5S_DEBUG -DH5P_DEBUG -DH5O_DEBUG -DH5MM_DEBUG -DH5MF_DEBUG
      -DH5I_DEBUG -DH5HL_DEBUG -DH5HG_DEBUG -DH5G_DEBUG -DH5F_DEBUG
      -DH5E_DEBUG -DH5D_DEBUG -DH5B_DEBUG -DH5AC_DEBUG -UNDEBUG
  )
ELSE (CMAKE_BUILD_TYPE MATCHES Debug)
  ADD_DEFINITIONS (-UH5_DEBUG_API -DNDEBUG)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

#-----------------------------------------------------------------------------
# Compiler specific flags : Shouldn't there be compiler tests for these
#-----------------------------------------------------------------------------
IF (NOT WIN32 AND NOT "${CMAKE_C_COMPILER_ID}" MATCHES "^XL$")
  include(CheckCCompilerFlag)
  
  check_c_compiler_flag(-std=c99 HAVE_STD_C99)
  check_c_compiler_flag(-fomit-frame-pointer HAVE_OMIT_FRAME_POINTER)
  check_c_compiler_flag(-finline-functions HAVE_INLINE_FUNCTIONS)
  check_c_compiler_flag(-fno-common HAVE_NO_COMMON)
  string(REGEX MATCH "clang" COMP_IS_CLANG "${CMAKE_C_COMPILER}")
  if(HAVE_STD_C99 AND NOT COMP_IS_CLANG)
    set(STD_C99 "-std=c99")
  endif()
  if(HAVE_OMIT_FRAME_POINTER)
    set(OMIT_FRAME_POINTER "-fomit-frame-pointer")
  endif()
  if(HAVE_INLINE_FUNCTIONS)
    set(INLINE_FUNCTIONS "-finline-functions")
  endif()
  if(HAVE_NO_COMMON)
    set(NO_COMMON "-fno-common")
  endif()

  SET (CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS} ${STD_C99} ${OMIT_FRAME_POINTER} ${INLINE_FUNCTIONS} ${NO_COMMON}")
ENDIF ()

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
  IF(WIN32)
    ADD_DEFINITIONS (-D_CRT_SECURE_NO_WARNINGS)
  ENDIF(WIN32)
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
# Option to Enable MPI Parallel
#-----------------------------------------------------------------------------
OPTION (HDF5_ENABLE_PARALLEL "Enable parallel build (requires MPI)" OFF)
IF (HDF5_ENABLE_PARALLEL)
  INCLUDE (FindMPI)
  INCLUDE_DIRECTORIES (${MPI_INCLUDE_PATH})
  IF (MPI_FOUND)
    SET (H5_HAVE_PARALLEL 1)
    # MPI checks, only do these if MPI_FOUND is true, otherwise they always fail
    # and once set, they are cached as false and not regenerated
    SET (CMAKE_REQUIRED_INCLUDES "${MPI_INCLUDE_PATH}/mpi.h" )
    SET (CMAKE_REQUIRED_LIBRARIES "${MPI_LIBRARIES}" )
    CHECK_FUNCTION_EXISTS (MPI_File_get_size H5_HAVE_MPI_GET_SIZE) 
    # Used by Fortran + MPI
    CHECK_SYMBOL_EXISTS (MPI_Comm_c2f "${MPI_INCLUDE_PATH}/mpi.h"  H5_HAVE_MPI_MULTI_LANG_Comm)
    CHECK_SYMBOL_EXISTS (MPI_Info_c2f "${MPI_INCLUDE_PATH}/mpi.h"  H5_HAVE_MPI_MULTI_LANG_Info)
  ENDIF (MPI_FOUND)
ENDIF (HDF5_ENABLE_PARALLEL)

# Parallel IO usage requires MPI to be Linked and Included
IF (H5_HAVE_PARALLEL)
  SET (LINK_LIBS ${LINK_LIBS} ${MPI_LIBRARIES})
  IF (MPI_LINK_FLAGS)
    SET (CMAKE_EXE_LINKER_FLAGS ${MPI_LINK_FLAGS} ${CMAKE_EXE_LINKER_FLAGS})
  ENDIF (MPI_LINK_FLAGS)
ENDIF (H5_HAVE_PARALLEL)

#-----------------------------------------------------------------------------
# Option to use 1.6.x API
#-----------------------------------------------------------------------------
OPTION (HDF5_USE_16_API_DEFAULT "Use the HDF5 1.6.x API by default" OFF)
SET (USE_16_API_DEFAULT 0)
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
      MESSAGE (FATAL_ERROR " ZLib is Required for ZLib support in HDF5")
    ENDIF (ZLIB_FOUND)
  ELSE (NOT H5_ZLIB_HEADER)
    # This project is being called from within another and ZLib is already configured
    SET (H5_HAVE_FILTER_DEFLATE 1)
    SET (H5_HAVE_ZLIB_H 1)
    SET (H5_HAVE_LIBZ 1)
  ENDIF (NOT H5_ZLIB_HEADER)
  CONFIGURE_FILE (${HDF5_SOURCE_DIR}/Resources/hdf5_zlib.h.in ${HDF5_BINARY_DIR}/hdf5_zlib.h @ONLY)
  SET (LINK_LIBS ${LINK_LIBS} ${ZLIB_LIBRARIES})
  INCLUDE_DIRECTORIES (${ZLIB_INCLUDE_DIRS})
  # MESSAGE (STATUS "Filter ZLIB is ON")
ENDIF (HDF5_ENABLE_Z_LIB_SUPPORT)

#-----------------------------------------------------------------------------
# Option for SzLib support
#-----------------------------------------------------------------------------
SET (CMAKE_MODULE_PATH ${HDF5_SOURCE_DIR}/Resources/CMake ${CMAKE_MODULE_PATH})
OPTION (HDF5_ENABLE_SZIP_SUPPORT "Use SZip Filter" OFF)
IF (HDF5_ENABLE_SZIP_SUPPORT)
  FIND_PACKAGE (SZIP REQUIRED)
  IF (SZIP_FOUND)
    SET (H5_HAVE_FILTER_SZIP 1)
    SET (H5_HAVE_SZLIB_H 1)
    SET (H5_HAVE_LIBSZ 1)
    SET (LINK_LIBS ${LINK_LIBS} ${SZIP_LIBRARIES})
    INCLUDE_DIRECTORIES (${SZIP_INCLUDE_DIRS})
    # MESSAGE (STATUS "SZIP_INCLUDE_DIRS: ${SZIP_INCLUDE_DIRS}")
    # MESSAGE (STATUS "SZIP_INCLUDE_DIR: ${SZIP_INCLUDE_DIR}")
    # MESSAGE (STATUS "Filter SZIP is ON")
    OPTION (HDF5_ENABLE_SZIP_ENCODING "Use SZip Encoding" OFF)
    IF (HDF5_ENABLE_SZIP_ENCODING)
      SET (H5_HAVE_SZIP_ENCODER 1)
    ENDIF (HDF5_ENABLE_SZIP_ENCODING)
  ELSE (SZIP_FOUND)
      MESSAGE (FATAL_ERROR "SZIP is Required for SZIP support in HDF5")
  ENDIF (SZIP_FOUND)
ENDIF (HDF5_ENABLE_SZIP_SUPPORT)

#-----------------------------------------------------------------------------
# Option to use PACKED BITS SUPPORT
#-----------------------------------------------------------------------------
OPTION (HDF5_USE_H5DUMP_PACKED_BITS "Use the PACKED BITS feature in h5dump" ON)
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
  ADD_EXECUTABLE(${XLATE_UTILITY} ${HDF5_SOURCE_DIR}/Resources/xlatefile.c)
ENDIF (HDF5_BUILD_TOOLS AND BUILD_TESTING)

#-----------------------------------------------------------------------------
# Dashboard and Testing Settings
#-----------------------------------------------------------------------------
IF(0)
OPTION (BUILD_TESTING "Build HDF5 Unit Testing" OFF)
ENDIF(0)
IF (BUILD_TESTING)
  SET (DART_TESTING_TIMEOUT 1200 CACHE INTEGER
       "Timeout in seconds for each test (default 1200=20minutes)")
  ENABLE_TESTING ()
  INCLUDE (CTest)
  IF (NOT HDF5_EXTERNALLY_CONFIGURED)
    ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/tools/lib ${PROJECT_BINARY_DIR}/tools/lib)
    ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/test      ${PROJECT_BINARY_DIR}/test)
    ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/perform   ${PROJECT_BINARY_DIR}/perform)
  ENDIF (NOT HDF5_EXTERNALLY_CONFIGURED)
  INCLUDE (${HDF5_SOURCE_DIR}/CTestConfig.cmake)
ENDIF (BUILD_TESTING)

#-----------------------------------------------------------------------------
# Option to build Fortran bindings/tests/examples
# Make sure this appears before the CONFIGURE_FILE step
# so that fortran name mangling is detected before writing H5pubconf.h
#-----------------------------------------------------------------------------
IF(0)
OPTION (HDF5_BUILD_FORTRAN "Build FORTRAN support" OFF)
ENDIF(0)
IF (HDF5_BUILD_FORTRAN)
  # Set default name mangling : overridden by Fortran detection in fortran dir
  SET (H5_FC_FUNC "H5_FC_FUNC(name,NAME) name ## _")
  SET (H5_FC_FUNC_ "H5_FC_FUNC_(name,NAME) name ## _")
  ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/fortran ${PROJECT_BINARY_DIR}/fortran)
ELSE (HDF5_BUILD_FORTRAN)
  SET (H5_FC_FUNC "H5_FC_FUNC(name,NAME) name ## _")
  SET (H5_FC_FUNC_ "H5_FC_FUNC_(name,NAME) name ## _")
ENDIF (HDF5_BUILD_FORTRAN)

#-----------------------------------------------------------------------------
# Option to build examples
#-----------------------------------------------------------------------------
IF(0)
OPTION (HDF5_BUILD_EXAMPLES  "Build HDF5 Library Examples" OFF)
IF (HDF5_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/examples ${PROJECT_BINARY_DIR}/examples)
ENDIF (HDF5_BUILD_EXAMPLES)
ENDIF(0)

#-----------------------------------------------------------------------------
# Option to build HDF5 C++ Library
#-----------------------------------------------------------------------------
OPTION (HDF5_BUILD_CPP_LIB  "Build HDF5 C++ Library" OFF)
IF (HDF5_BUILD_CPP_LIB)
  ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/c++ ${PROJECT_BINARY_DIR}/c++)
ENDIF (HDF5_BUILD_CPP_LIB)

#-----------------------------------------------------------------------------
# Option to build HDF5 Tools
#-----------------------------------------------------------------------------
IF(0)
OPTION (HDF5_BUILD_TOOLS  "Build HDF5 Tools" OFF)
IF (HDF5_BUILD_TOOLS)
  ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/tools ${PROJECT_BINARY_DIR}/tools)
ENDIF (HDF5_BUILD_TOOLS)
ENDIF(0)

#-----------------------------------------------------------------------------
# Option to build High Level API's
#-----------------------------------------------------------------------------
OPTION (HDF5_BUILD_HL_LIB  "Build HIGH Level HDF5 Library" OFF)
IF (HDF5_BUILD_HL_LIB)
  SET (H5_INCLUDE_HL 1)
  ADD_SUBDIRECTORY (${HDF5_SOURCE_DIR}/hl ${PROJECT_BINARY_DIR}/hl)
ENDIF (HDF5_BUILD_HL_LIB)

#-----------------------------------------------------------------------------
# Generate the H5pubconf.h file containing user settings needed by compilation
#-----------------------------------------------------------------------------
CONFIGURE_FILE (${HDF5_SOURCE_DIR}/Resources/H5pubconf.h.in ${HDF5_BINARY_DIR}/H5pubconf.h @ONLY)

#-----------------------------------------------------------------------------
# Generate the HDF5Config file containing build settings used by externaly
#-----------------------------------------------------------------------------
CONFIGURE_FILE (
  ${HDF5_SOURCE_DIR}/Resources/HDF5Config.cmake.in
  ${HDF5_BINARY_DIR}/vtkHDF5Config.cmake @ONLY
)

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
IF (NOT VTK_INSTALL_NO_DEVELOPMENT)
INSTALL (
    FILES
        ${PROJECT_BINARY_DIR}/H5pubconf.h
    DESTINATION
        ${VTK_INSTALL_INCLUDE_DIR}/vtkhdf5
)
ENDIF (NOT VTK_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
# This is not a full HDF5, so do not provide HDF5-config.cmake
IF(0)
INSTALL (
    EXPORT
        ${HDF5_EXPORTED_TARGETS}
    DESTINATION
        lib
    FILE
        HDF5-config.cmake
)

# The install rule below copies compiled module files to the install location
# using a "code" rule. The code string should not be broken up, it must be on one line
IF (HDF5_BUILD_FORTRAN)
    INSTALL ( CODE
        "EXECUTE_PROCESS (COMMAND \"${CMAKE_COMMAND}\" -E copy_directory \"${CMAKE_Fortran_MODULE_DIRECTORY}/\${BUILD_TYPE}\" \"${CMAKE_INSTALL_PREFIX}/fortran\")"
    )
ENDIF (HDF5_BUILD_FORTRAN)

#-----------------------------------------------------------------------------
# Export all exported targets to the build tree for use by parent project
#-----------------------------------------------------------------------------
EXPORT (
    TARGETS
        ${HDF5_LIBRARIES_TO_EXPORT} ${HDF5_LIB_DEPENDENCIES}
    FILE
        HDF5-config.cmake
)
ENDIF(0)
