CMAKE_MINIMUM_REQUIRED(VERSION 1.6)
PROJECT(VTK)
INCLUDE_REGULAR_EXPRESSION("^(lex|vtk|png|j).*$")

# Don't build anything unless the version of CMake is high enough.
# The matching ELSE/ENDIF should be the last lines in the file.
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.5)

#-----------------------------------------------------------------------------
# VTK version number.  An even minor number corresponds to releases.
SET(VTK_MAJOR_VERSION 4)
SET(VTK_MINOR_VERSION 2)
SET(VTK_BUILD_VERSION 6)
SET(VTK_VERSION
    "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")

#-----------------------------------------------------------------------------
# Output directories.
SET(LIBRARY_OUTPUT_PATH ${VTK_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all libraries.")
SET(EXECUTABLE_OUTPUT_PATH ${VTK_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
SET(VTK_LIBRARY_DIR ${LIBRARY_OUTPUT_PATH}/${CMAKE_CFG_INTDIR})
SET(VTK_EXECUTABLE_DIR ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR})
SET(CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

# Put "." in the rpath of all VTK shared libraries.
LINK_DIRECTORIES(.)

#-----------------------------------------------------------------------------
# Load some macros.
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkDependentOption.cmake)
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkThirdParty.cmake)

#-----------------------------------------------------------------------------
# Platform configuration tests.
INCLUDE(${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityC.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityCXX.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)

SET(VTK_SIZEOF_CHAR   ${CMAKE_SIZEOF_CHAR})
SET(VTK_SIZEOF_DOUBLE ${CMAKE_SIZEOF_DOUBLE})
SET(VTK_SIZEOF_FLOAT  ${CMAKE_SIZEOF_FLOAT})
SET(VTK_SIZEOF_INT    ${CMAKE_SIZEOF_INT})
SET(VTK_SIZEOF_LONG   ${CMAKE_SIZEOF_LONG})
SET(VTK_SIZEOF_SHORT  ${CMAKE_SIZEOF_SHORT})
CHECK_TYPE_SIZE("long long" VTK_SIZEOF_LONG_LONG)

IF("VTK_COMPILER_HAS_BOOL" MATCHES "^VTK_COMPILER_HAS_BOOL$")
  MESSAGE(STATUS "Checking support for C++ type bool")
  TRY_COMPILE(VTK_COMPILER_HAS_BOOL
              ${VTK_BINARY_DIR}/CMakeTmp/Bool
              ${VTK_SOURCE_DIR}/CMake/vtkTestBoolType.cxx
              OUTPUT_VARIABLE OUTPUT)
  IF(VTK_COMPILER_HAS_BOOL)
    MESSAGE(STATUS "Checking support for C++ type bool -- yes")
    SET(VTK_COMPILER_HAS_BOOL 1 CACHE INTERNAL "Support for C++ type bool")
  ELSE(VTK_COMPILER_HAS_BOOL)
    MESSAGE(STATUS "Checking support for C++ type bool -- no")
    SET(VTK_COMPILER_HAS_BOOL 0 CACHE INTERNAL "Support for C++ type bool")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeError.log 
      "Determining if the C++ compiler supports type bool "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_COMPILER_HAS_BOOL)
ENDIF("VTK_COMPILER_HAS_BOOL" MATCHES "^VTK_COMPILER_HAS_BOOL$")

IF("VTK_EXPLICIT_TEMPLATES" MATCHES "^VTK_EXPLICIT_TEMPLATES")
  MESSAGE(STATUS "Checking support for C++ explicit template instantiation")
  TRY_COMPILE(VTK_EXPLICIT_TEMPLATES
              ${VTK_BINARY_DIR}
              ${VTK_SOURCE_DIR}/CMake/vtkTestExplicitInstantiation.cxx
              OUTPUT_VARIABLE OUTPUT)
  IF(VTK_EXPLICIT_TEMPLATES)
    MESSAGE(STATUS "Checking support for C++ explicit template instantiation -- yes")
    SET(VTK_EXPLICIT_TEMPLATES 1 CACHE INTERNAL "Support for C++ explict templates")
  ELSE(VTK_EXPLICIT_TEMPLATES)
    MESSAGE(STATUS "Checking support for C++ explict templates instantiation -- no")
    SET(VTK_EXPLICIT_TEMPLATES_BOOL 0 CACHE INTERNAL "Support for C++ explict templates")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeError.log 
      "Determining if the C++ compiler supports explict template instantiation "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_EXPLICIT_TEMPLATES)
ENDIF("VTK_EXPLICIT_TEMPLATES" MATCHES "^VTK_EXPLICIT_TEMPLATES")

IF(CMAKE_COMPILER_IS_GNUCXX AND WIN32)
  # GCC on Cygwin cannot do explicit template instantiation correctly with
  # DLLs.
  SET(CMAKE_NO_EXPLICIT_TEMPLATE_INSTANTIATION 1)
ENDIF(CMAKE_COMPILER_IS_GNUCXX AND WIN32)

#-----------------------------------------------------------------------------
# Load the VTK CMake extension modules.

# Import the VTK_COMPILE_CMAKE_EXTENSIONS and VTK_LOAD_CMAKE_EXTENSIONS macros.
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkCompileCMakeExtensions.cmake)
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkLoadCMakeExtensions.cmake)

# Compile the extensions into the build tree and load them.
VTK_COMPILE_CMAKE_EXTENSIONS(${VTK_SOURCE_DIR}/CMake ${VTK_BINARY_DIR}/CMake
                             VTK_CMAKE_EXTENSIONS_COMPILED)
IF(VTK_CMAKE_EXTENSIONS_COMPILED)
  VTK_LOAD_CMAKE_EXTENSIONS(${VTK_BINARY_DIR}/CMake)
ENDIF(VTK_CMAKE_EXTENSIONS_COMPILED)

#-----------------------------------------------------------------------------
# Choose static or shared libraries.
OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
SET(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

#-----------------------------------------------------------------------------
# Determine the set of VTK kits that should be built.
OPTION(VTK_USE_RENDERING "Build the vtkRendering kit.  Needed for displaying data." ON)
OPTION(VTK_USE_PATENTED
       "Build the vtkPatented kit.  These classes are patented and may require a license to use."
       OFF)
VTK_DEPENDENT_OPTION(VTK_USE_HYBRID "Build the vtkHybrid kit." OFF
                     "VTK_USE_RENDERING" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_PARALLEL "Build the vtkParallel kit." OFF
                     "VTK_USE_RENDERING" OFF)

SET(VTK_KITS COMMON FILTERING IO GRAPHICS IMAGING)
IF(VTK_USE_RENDERING)
  SET(VTK_KITS ${VTK_KITS} RENDERING)
ENDIF(VTK_USE_RENDERING)
IF(VTK_USE_HYBRID)
  SET(VTK_KITS ${VTK_KITS} HYBRID)
ENDIF(VTK_USE_HYBRID)
IF(VTK_USE_PARALLEL)
  SET(VTK_KITS ${VTK_KITS} PARALLEL)
ENDIF(VTK_USE_PARALLEL)
IF(VTK_USE_PATENTED)
  SET(VTK_KITS ${VTK_KITS} PATENTED)
ENDIF(VTK_USE_PATENTED)

#-----------------------------------------------------------------------------
# Configure Dart testing support.
INCLUDE(${CMAKE_ROOT}/Modules/Dart.cmake)
MARK_AS_ADVANCED(BUILD_TESTING DART_ROOT TCL_TCLSH)
IF(BUILD_TESTING)
  ENABLE_TESTING()
ENDIF(BUILD_TESTING)

#-----------------------------------------------------------------------------
# Determine the set of language wrappers that should be built.
OPTION(VTK_WRAP_TCL "Wrap VTK classes into the TCL language." OFF)
OPTION(VTK_WRAP_PYTHON "Wrap VTK classes into the Python language." OFF)
OPTION(VTK_WRAP_JAVA "Wrap VTK classes into the Java language." OFF)

# Python requires shared libraries.
IF(VTK_WRAP_PYTHON)
  IF(NOT BUILD_SHARED_LIBS)
    MESSAGE(SEND_ERROR "VTK_WRAP_PYTHON requires BUILD_SHARED_LIBS to be ON.")
    SET(VTK_WRAP_PYTHON 0)
  ENDIF(NOT BUILD_SHARED_LIBS)
ENDIF(VTK_WRAP_PYTHON)

# Java requires shared libraries on Windows.
IF(VTK_WRAP_JAVA)
  IF(WIN32)
    IF(NOT BUILD_SHARED_LIBS)
      MESSAGE(SEND_ERROR "VTK_WRAP_JAVA requires BUILD_SHARED_LIBS to be ON.")
      SET(VTK_WRAP_JAVA 0)
    ENDIF(NOT BUILD_SHARED_LIBS)
  ENDIF(WIN32)
ENDIF(VTK_WRAP_JAVA)

SET(VTK_LANGUAGES "")
IF(VTK_WRAP_TCL)
  SET(VTK_LANGUAGES ${VTK_LANGUAGES} TCL)
ENDIF(VTK_WRAP_TCL)
IF(VTK_WRAP_PYTHON)
  SET(VTK_LANGUAGES ${VTK_LANGUAGES} PYTHON)
ENDIF(VTK_WRAP_PYTHON)
IF(VTK_WRAP_JAVA)
  SET(VTK_LANGUAGES ${VTK_LANGUAGES} JAVA)
ENDIF(VTK_WRAP_JAVA)

#-----------------------------------------------------------------------------
# Dispatch the build into the proper subdirectories.

# Utility libraries and executables.
SUBDIRS(Wrapping Utilities)

# Kits.
SUBDIRS(Common Filtering Imaging Graphics IO)
IF(VTK_USE_RENDERING)
  SUBDIRS(Rendering)
ENDIF(VTK_USE_RENDERING)
IF(VTK_USE_HYBRID)
  SUBDIRS(Hybrid)
ENDIF(VTK_USE_HYBRID)
IF(VTK_USE_PATENTED)
  SUBDIRS(Patented)
ENDIF(VTK_USE_PATENTED)
IF(VTK_USE_PARALLEL)
  SUBDIRS(Parallel)
ENDIF(VTK_USE_PARALLEL)

# Wrapping.
IF(VTK_WRAP_TCL)
  SUBDIRS(Wrapping/Tcl)
ENDIF(VTK_WRAP_TCL)
IF(VTK_WRAP_PYTHON)
  SUBDIRS(Wrapping/Python)
ENDIF(VTK_WRAP_PYTHON)
IF(VTK_WRAP_JAVA)
  SUBDIRS(Wrapping/Java)
ENDIF(VTK_WRAP_JAVA)

# Testing.
IF(BUILD_TESTING)
  SUBDIRS(Common/Testing Filtering/Testing Graphics/Testing Imaging/Testing 
          IO/Testing)
  IF(VTK_USE_RENDERING)
    SUBDIRS(Rendering/Testing)
  ENDIF(VTK_USE_RENDERING)
  IF(VTK_USE_HYBRID)
    SUBDIRS(Hybrid/Testing)
  ENDIF(VTK_USE_HYBRID)
  IF(VTK_USE_PARALLEL)
    SUBDIRS(Parallel/Testing)
  ENDIF(VTK_USE_PARALLEL)
  IF(VTK_USE_PATENTED)
    SUBDIRS(Patented/Testing)
  ENDIF(VTK_USE_PATENTED)
ENDIF(BUILD_TESTING)

# Examples.
IF(BUILD_EXAMPLES)
  SUBDIRS(Examples)
ENDIF(BUILD_EXAMPLES)

#-----------------------------------------------------------------------------
# Determine GUI.
SET(VTK_USE_X_OPTIONAL 0)
SET(VTK_USE_X_FORCE 0)
IF(CMAKE_HAS_X)
  IF(APPLE)
    SET(VTK_USE_X_OPTIONAL 1)
  ENDIF(APPLE)
  IF(CYGWIN)
    SET(VTK_USE_X_OPTIONAL 1)
  ENDIF(CYGWIN)
  SET(VTK_USE_X_FORCE ${VTK_USE_RENDERING})
ENDIF(CMAKE_HAS_X)

VTK_DEPENDENT_OPTION(VTK_USE_CARBON "Build classes using Carbon API." ON
                     "APPLE;VTK_USE_RENDERING" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_COCOA "Build classes using Cocoa API." OFF
                     "APPLE;VTK_USE_RENDERING" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_X
                     "Build classes for the X11 window system." OFF
                     "CMAKE_HAS_X;VTK_USE_RENDERING;VTK_USE_X_OPTIONAL"
                     "${VTK_USE_X_FORCE}")

IF(VTK_USE_CARBON AND VTK_USE_COCOA)
  MESSAGE(SEND_ERROR "Only one of VTK_USE_CARBON and VTK_USE_COCOA may be ON.")
  SET(VTK_USE_CARBON 0)
  SET(VTK_USE_COCOA 0)
ENDIF(VTK_USE_CARBON AND VTK_USE_COCOA)

#-----------------------------------------------------------------------------
# Select a streams library.
INCLUDE (${CMAKE_ROOT}/Modules/CheckIncludeFileCXX.cmake)
CHECK_INCLUDE_FILE_CXX("iosfwd" VTK_HAVE_ANSI_STREAMS)
CHECK_INCLUDE_FILE_CXX("iostream.h" VTK_HAVE_OLD_STREAMS)
IF(VTK_HAVE_OLD_STREAMS)
  # Some compilers have iostream.h but not strstream.h
  # or strstrea.h.  Treat these as not having old streams.
  CHECK_INCLUDE_FILE_CXX("strstrea.h" VTK_HAVE_OLD_STRSTREA_H)
  CHECK_INCLUDE_FILE_CXX("strstream.h" VTK_HAVE_OLD_STRSTREAM_H)
  IF(NOT VTK_HAVE_OLD_STRSTREAM_H)
    IF(NOT VTK_HAVE_OLD_STRSTREA_H)
      SET(VTK_HAVE_OLD_STREAMS 0)
    ENDIF(NOT VTK_HAVE_OLD_STRSTREA_H)
  ENDIF(NOT VTK_HAVE_OLD_STRSTREAM_H)
ENDIF(VTK_HAVE_OLD_STREAMS)

IF(VTK_HAVE_ANSI_STREAMS AND VTK_HAVE_OLD_STREAMS)
  # Have both old and new streams.  Provide the option.
  SET(VTK_USE_ANSI_STDLIB_DEFAULT ON)
  IF(NOT CMAKE_COMPILER_IS_GNUCXX)
    IF(CMAKE_BUILD_TOOL MATCHES "devenv")
    ELSE(CMAKE_BUILD_TOOL MATCHES "devenv")
      SET(VTK_USE_ANSI_STDLIB_DEFAULT OFF)
    ENDIF(CMAKE_BUILD_TOOL MATCHES "devenv")
  ENDIF(NOT CMAKE_COMPILER_IS_GNUCXX)
  OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library."
    ${VTK_USE_ANSI_STDLIB_DEFAULT})
  MARK_AS_ADVANCED(VTK_USE_ANSI_STDLIB)
ELSE(VTK_HAVE_ANSI_STREAMS AND VTK_HAVE_OLD_STREAMS)
  IF(VTK_HAVE_ANSI_STREAMS)
    # Have only new streams.  Use them.
    SET(VTK_USE_ANSI_STDLIB ON)
  ELSE(VTK_HAVE_ANSI_STREAMS)
    # Have only old streams.  Use them.
    SET(VTK_USE_ANSI_STDLIB OFF)
  ENDIF(VTK_HAVE_ANSI_STREAMS)
ENDIF(VTK_HAVE_ANSI_STREAMS AND VTK_HAVE_OLD_STREAMS)

#-----------------------------------------------------------------------------
# Provide a few configuration options.
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)
OPTION(VTK_USE_64BIT_IDS "Build VTK with 64 bit ids" OFF)
OPTION(VTK_DEBUG_LEAKS "Build leak checking support into VTK." OFF)
MARK_AS_ADVANCED(VTK_DEBUG_LEAKS VTK_USE_64BIT_IDS)

VTK_DEPENDENT_OPTION(VTK_USE_MANGLED_MESA "Use mangled Mesa with OpenGL." OFF
                     "VTK_USE_RENDERING" OFF)
VTK_DEPENDENT_OPTION(VTK_OPENGL_HAS_OSMESA 
                     "The opengl library being used supports off screen Mesa calls." OFF
                     "VTK_USE_RENDERING;UNIX" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_MPI
                     "Use Message Passing Interface (MPI) library for parallel support." OFF
                     "VTK_USE_PARALLEL" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_VOLUMEPRO "Build VTK with VolumePro support." OFF
                     "VTK_USE_RENDERING" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_MATROX_IMAGING
                     "Use Matrox Imaging Library for video input." OFF
                     "VTK_USE_HYBRID;WIN32" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_VIDEO_FOR_WINDOWS
                     "Use Video for Windows for video input." OFF
                     "VTK_USE_HYBRID;WIN32" OFF)
IF(VTK_USE_COCOA)
  SET(VTK_DISABLE_TK_INIT 1)
ELSE(VTK_USE_COCOA)
  VTK_DEPENDENT_OPTION(VTK_DISABLE_TK_INIT
                       "Disables the automatic initialization of Tk widgets when loading the rendering library." OFF
                       "VTK_WRAP_TCL;VTK_USE_RENDERING" OFF)
  MARK_AS_ADVANCED(VTK_DISABLE_TK_INIT)
ENDIF(VTK_USE_COCOA)
MARK_AS_ADVANCED(VTK_OPENGL_HAS_OSMESA
                 VTK_USE_MANGLED_MESA
                 VTK_USE_MATROX_IMAGING
                 VTK_USE_MPI
                 VTK_USE_VIDEO_FOR_WINDOWS
                 VTK_USE_VOLUMEPRO)

#-----------------------------------------------------------------------------
# Provide options to use system versions of third-party libraries.
VTK_THIRD_PARTY_OPTION(ZLIB zlib)
VTK_THIRD_PARTY_OPTION(JPEG jpeg)
VTK_THIRD_PARTY_OPTION(PNG  png)
VTK_THIRD_PARTY_OPTION(TIFF tiff)
VTK_THIRD_PARTY_OPTION(EXPAT expat)

#-----------------------------------------------------------------------------
# VTK requires special compiler flags on some platforms.
IF(CMAKE_COMPILER_IS_GNUCXX)
  # A GCC compiler.  Quiet warning about strstream deprecation.
  SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -Wno-deprecated")
  IF(WIN32)
    # The platform is gcc on cygwin.
    SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -mwin32")
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -mwin32")
  ENDIF(WIN32)
  IF(MINGW)
    LINK_LIBRARIES(-lgdi32)
  ENDIF(MINGW)
  IF(CMAKE_SYSTEM MATCHES "SunOS.*")
    # Disable warnings that occur in X11 headers.
    IF(DART_ROOT AND BUILD_TESTING)
      SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -Wno-unknown-pragmas")
      SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -Wno-unknown-pragmas")
    ENDIF(DART_ROOT AND BUILD_TESTING)
  ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_ANSI_CFLAGS)
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} ${CMAKE_ANSI_CFLAGS}")
  ENDIF(CMAKE_ANSI_CFLAGS)
  IF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
     SET(VTK_REQUIRED_CXX_FLAGS
         "${VTK_REQUIRED_CXX_FLAGS} -timplicit_local -no_implicit_include")
  ENDIF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
  #silence duplicate symbol warnings on AIX
  IF(CMAKE_SYSTEM MATCHES "AIX.*")
    SET(VTK_REQUIRED_EXE_LINKER_FLAGS "${VTK_REQUIRED_EXE_LINKER_FLAGS} -bhalt:5")
  ENDIF(CMAKE_SYSTEM MATCHES "AIX.*")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(APPLE)
  SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -no-cpp-precomp")
  SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -no-cpp-precomp")
  IF(VTK_USE_CARBON)
    SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -fpascal-strings")
  ENDIF(VTK_USE_CARBON)
  IF(BUILD_SHARED_LIBS)
    SET(VTK_APPLE_SHARED_FLAGS_NEEDED 1)
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(APPLE)

IF(VTK_USE_X)
  IF(CMAKE_X_CFLAGS)
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} ${CMAKE_X_CFLAGS}")
    SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} ${CMAKE_X_CFLAGS}")
  ENDIF(CMAKE_X_CFLAGS)
ENDIF(VTK_USE_X)

#-----------------------------------------------------------------------------
# Configure OpenGL support.
IF(VTK_USE_RENDERING)
  INCLUDE(${CMAKE_ROOT}/Modules/FindOpenGL.cmake)
ENDIF(VTK_USE_RENDERING)

#-----------------------------------------------------------------------------
# Configure Mangled MESA support.
IF(VTK_USE_MANGLED_MESA)
  INCLUDE(${VTK_SOURCE_DIR}/CMake/FindMangledMesa.cmake)

  MARK_AS_ADVANCED(MANGLED_MESA_INCLUDE_DIR MANGLED_MESA_LIBRARY
                   MANGLED_OSMESA_INCLUDE_DIR MANGLED_OSMESA_LIBRARY)

  IF(MANGLED_MESA_INCLUDE_DIR)
    USE_MANGLED_MESA(${MANGLED_MESA_INCLUDE_DIR}/GL
                     ${VTK_BINARY_DIR}/MangleMesaInclude)
  ENDIF(MANGLED_MESA_INCLUDE_DIR)
ENDIF(VTK_USE_MANGLED_MESA)

#-----------------------------------------------------------------------------
# Configure Off-Screen MESA support.
IF(VTK_OPENGL_HAS_OSMESA)
  INCLUDE(${VTK_SOURCE_DIR}/CMake/FindOSMesa.cmake)
  MARK_AS_ADVANCED(OSMESA_INCLUDE_DIR OSMESA_LIBRARY)
ENDIF(VTK_OPENGL_HAS_OSMESA)

# Off-Screen MESA cannot be used with Mangled MESA.
IF(VTK_OPENGL_HAS_OSMESA AND VTK_USE_MANGLED_MESA)
  MESSAGE(SEND_ERROR
    "Off-Screen MESA cannot be used with Mangled MESA.  Turn off either "
    "VTK_OPENGL_HAS_OSMESA or VTK_USE_MANGLED_MESA.")
ENDIF(VTK_OPENGL_HAS_OSMESA AND VTK_USE_MANGLED_MESA)

#-----------------------------------------------------------------------------
# Configure VolumePro support.
IF(VTK_USE_VOLUMEPRO)
  INCLUDE(${VTK_SOURCE_DIR}/CMake/FindVLI.cmake)
  MARK_AS_ADVANCED(VLI_LIBRARY_FOR_VG500 VLI_INCLUDE_PATH_FOR_VG500
                   VLI_LIBRARY_FOR_VP1000 VLI_INCLUDE_PATH_FOR_VP1000)
  IF(VLI_LIBRARY_FOR_VG500 AND VLI_INCLUDE_PATH_FOR_VG500)
    SET(VTK_HAVE_VG500 ON)
  ENDIF(VLI_LIBRARY_FOR_VG500 AND VLI_INCLUDE_PATH_FOR_VG500)
  IF(VLI_LIBRARY_FOR_VP1000 AND VLI_INCLUDE_PATH_FOR_VP1000)
    SET(VTK_HAVE_VP1000 ON)
  ENDIF(VLI_LIBRARY_FOR_VP1000 AND VLI_INCLUDE_PATH_FOR_VP1000)
ENDIF(VTK_USE_VOLUMEPRO)

#-----------------------------------------------------------------------------
# Configure Matrox Imaging support.
IF(VTK_USE_MATROX_IMAGING)
  FIND_LIBRARY(MIL_LIBRARY MIL
    "C:/Program Files/Matrox Imaging/mil/library/winnt/msc/dll"
    "C:/Program Files/Matrox Imaging/mil/library/windows/msc/dll"
    )
  FIND_PATH(MIL_INCLUDE_PATH mil.h
    "C:/Program Files/Matrox Imaging/mil/include"
    )
ENDIF(VTK_USE_MATROX_IMAGING)

#-----------------------------------------------------------------------------
# Configure MPI testing support.
IF(VTK_USE_MPI AND BUILD_TESTING)
  FIND_PROGRAM(VTK_MPIRUN_EXE NAMES mpirun lamexec)
  SET(VTK_MPI_PREFLAGS "-np 2" CACHE STRING "Flags used by MPI start program.")
  SET(VTK_MPI_POSTFLAGS "" CACHE STRING "Flags used by MPI start program.")
  SET(VTK_MPI_MAX_NUMPROCS "2" CACHE STRING "Maximum number of processors available to run parallel applications.")
  MARK_AS_ADVANCED(VTK_MPIRUN_EXE VTK_MPI_PREFLAGS VTK_MPI_POSTFLAGS VTK_MPI_MAX_NUMPROCS)
  SEPARATE_ARGUMENTS(VTK_MPI_PREFLAGS)
  SEPARATE_ARGUMENTS(VTK_MPI_POSTFLAGS)
ENDIF(VTK_USE_MPI AND BUILD_TESTING)

#-----------------------------------------------------------------------------
# VTK utility script locations.
SET(VTK_DOXYGEN_HOME ${VTK_SOURCE_DIR}/Utilities/Doxygen)
SET(VTK_HEADER_TESTING_PY ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py)
SET(VTK_FIND_STRING_TCL ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl)
SET(VTK_PRINT_SELF_CHECK_TCL ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl)
SET(VTK_RT_IMAGE_TEST_TCL ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl)
IF(VTK_USE_PARALLEL)
  SET(VTK_PRT_IMAGE_TEST_TCL ${VTK_SOURCE_DIR}/Common/Testing/Tcl/prtImageTest.tcl)
ENDIF(VTK_USE_PARALLEL)

#-----------------------------------------------------------------------------
# Configure Tcl wrapping support.
IF(VTK_WRAP_TCL)
  INCLUDE(${CMAKE_ROOT}/Modules/FindTCL.cmake)

  # Hide useless settings provided by FindTCL.
  FOREACH(entry TCL_LIBRARY_DEBUG
                TK_LIBRARY_DEBUG
                TCL_STUB_LIBRARY
                TCL_STUB_LIBRARY_DEBUG
                TK_STUB_LIBRARY
                TK_STUB_LIBRARY_DEBUG
                TK_WISH)
    SET(${entry} "${${entry}}" CACHE INTERNAL "This value is not used by VTK.")
  ENDFOREACH(entry)

  # Wrapping executables.
  UTILITY_SOURCE(VTK_WRAP_TCL_EXE vtkWrapTcl Wrapping vtkWrapTcl.c)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping)
  MARK_AS_ADVANCED(VTK_WRAP_TCL_EXE VTK_WRAP_HINTS)

  # VTK tcl executables.
  SET(VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
  IF(VTK_USE_PARALLEL AND VTK_USE_MPI)
    SET(PVTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/pvtk)
  ENDIF(VTK_USE_PARALLEL AND VTK_USE_MPI)

  # Tcl package location.
  SET(VTK_TCL_HOME ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ENDIF(VTK_WRAP_TCL)

#-----------------------------------------------------------------------------
# Configure Python wrapping support.
IF(VTK_WRAP_PYTHON)
  INCLUDE(${CMAKE_ROOT}/Modules/FindPythonLibs.cmake)

  # Wrapping executables.
  UTILITY_SOURCE(VTK_WRAP_PYTHON_EXE vtkWrapPython Wrapping vtkWrapPython.c)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping)
  MARK_AS_ADVANCED(VTK_WRAP_PYTHON_EXE VTK_WRAP_HINTS)

  # VTK tcl executables.
  SET(VTK_PYTHON_EXE ${EXECUTABLE_OUTPUT_PATH}/vtkpython)
  IF(VTK_USE_PARALLEL AND VTK_USE_MPI)
    SET(PVTK_PYTHON_EXE ${EXECUTABLE_OUTPUT_PATH}/pvtkpython)
  ENDIF(VTK_USE_PARALLEL AND VTK_USE_MPI)

  # Safety check.
  IF(NOT PYTHON_DEBUG_LIBRARY)
    SET(PYTHON_DEBUG_LIBRARY ${PYTHON_LIBRARY})
    IF(WIN32)
      MESSAGE("Cannot find Python debug library.  The regular Python library "
              "will be used.  This means that when using debug build of VTK, "
              "python examples might not run.  If you use release build or "
              "you do not care about python working, set PYTHON_DEBUG_LIBRARY "
              "to the same file as PYTHON_LIBRARY.")
    ENDIF(WIN32)
  ENDIF(NOT PYTHON_DEBUG_LIBRARY)
ENDIF(VTK_WRAP_PYTHON)

#-----------------------------------------------------------------------------
# Configure Java wrapping support.
IF(VTK_WRAP_JAVA)
  INCLUDE(${CMAKE_ROOT}/Modules/FindJava.cmake)
  INCLUDE(${CMAKE_ROOT}/Modules/FindJNI.cmake)

  # Wrapping executables.
  UTILITY_SOURCE(VTK_WRAP_JAVA_EXE vtkWrapJava Wrapping vtkWrapJava.c)
  UTILITY_SOURCE(VTK_PARSE_JAVA_EXE vtkParseJava Wrapping vtkParseJava.c)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping)
  MARK_AS_ADVANCED(VTK_WRAP_JAVA_EXE VTK_PARSE_JAVA_EXE VTK_WRAP_HINTS)

  # Java package location.
  SET(VTK_JAVA_JAR ${LIBRARY_OUTPUT_PATH}/vtk.jar)
  SET(VTK_JAVA_HOME ${VTK_BINARY_DIR}/java/vtk)
  MAKE_DIRECTORY(${VTK_JAVA_HOME})
ENDIF(VTK_WRAP_JAVA)

#-----------------------------------------------------------------------------
# Configure the Tk library for vtkRendering.
IF(VTK_USE_RENDERING)
  IF(VTK_WRAP_TCL)
    SET(VTK_INCLUDE_NEED_TK 1)
  ENDIF(VTK_WRAP_TCL)
  IF(VTK_WRAP_PYTHON)
    SET(VTK_INCLUDE_NEED_TK 1)
  ENDIF(VTK_WRAP_PYTHON)
ENDIF(VTK_USE_RENDERING)

IF(VTK_INCLUDE_NEED_TK)
  # Need Tk sources on windows
  IF(WIN32)
    FIND_PATH(TK_XLIB_PATH X11/Xlib.h ${TK_INCLUDE_PATH} 
              ${TK_INCLUDE_PATH}/../xlib)
    MARK_AS_ADVANCED(TK_XLIB_PATH)
  ENDIF(WIN32)
  # Need Tk headers and libraries for python TK widgets
  IF(NOT VTK_WRAP_TCL)
    INCLUDE(${CMAKE_ROOT}/Modules/FindTCL.cmake)
  ENDIF(NOT VTK_WRAP_TCL)
ENDIF(VTK_INCLUDE_NEED_TK)

# Need Tk internal headers for Tk initialization.
SET(VTK_RENDERING_NEED_TK_INTERNAL ${VTK_INCLUDE_NEED_TK})
IF(VTK_DISABLE_TK_INIT)
  SET(VTK_RENDERING_NEED_TK_INTERNAL 0)
ENDIF(VTK_DISABLE_TK_INIT)

IF(VTK_INCLUDE_NEED_TK)
  # Need Tk Internal headers to include tk.h on Cocoa
  IF(VTK_USE_RENDERING AND VTK_USE_COCOA)
     SET(VTK_RENDERING_NEED_TK_INTERNAL 1)
  ENDIF(VTK_USE_RENDERING AND VTK_USE_COCOA)
ENDIF(VTK_INCLUDE_NEED_TK)

IF(VTK_RENDERING_NEED_TK_INTERNAL AND TK_LIBRARY)
  IF(APPLE)
    FIND_PATH(TK_INTERNAL_PATH tkMacOSXInt.h
      ${VTK_SOURCE_DIR}/Rendering/tkInternals/tk84OSX)
  ELSE(APPLE)
    IF(TK_LIBRARY MATCHES tk82)
      FIND_PATH(TK_INTERNAL_PATH tkWinInt.h
        ${VTK_SOURCE_DIR}/Rendering/tkInternals/tk82)
    ENDIF(TK_LIBRARY MATCHES tk82)
    IF(TK_LIBRARY MATCHES tk83)
      FIND_PATH(TK_INTERNAL_PATH tkWinInt.h
        ${VTK_SOURCE_DIR}/Rendering/tkInternals/tk83)
    ENDIF(TK_LIBRARY MATCHES tk83)
    IF(TK_LIBRARY MATCHES tk84)
      FIND_PATH(TK_INTERNAL_PATH tkWinInt.h
        ${VTK_SOURCE_DIR}/Rendering/tkInternals/tk84)
    ENDIF(TK_LIBRARY MATCHES tk84)
  ENDIF(APPLE)
ENDIF(VTK_RENDERING_NEED_TK_INTERNAL AND TK_LIBRARY)

#-----------------------------------------------------------------------------
# Configure the python executable for use by testing.

# Python executable is used by some tests whether VTK_WRAP_PYTHON is
# on or not.  do not add a VTK_WRAP_PYTHON to this if.
SET(VTK_NEED_PYTHON_EXECUTABLE 0)
IF(BUILD_TESTING AND DART_ROOT)
  SET(VTK_NEED_PYTHON_EXECUTABLE 1)
ENDIF(BUILD_TESTING AND DART_ROOT)

# If VTK_WRAP_PYTHON is on, then we need python executable to compile
# scripts.
IF(VTK_WRAP_PYTHON)
  SET(VTK_NEED_PYTHON_EXECUTABLE 1)
ENDIF(VTK_WRAP_PYTHON)

IF(VTK_NEED_PYTHON_EXECUTABLE)
  FIND_PROGRAM(PYTHON_EXECUTABLE
    NAMES python2.3 python2.2 python2.1 python2.0 python1.6 python1.5 python
    PATHS
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.3\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.2\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.1\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.0\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.6\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.5\\InstallPath]
)
  MARK_AS_ADVANCED(PYTHON_EXECUTABLE)
ENDIF(VTK_NEED_PYTHON_EXECUTABLE)

#-----------------------------------------------------------------------------
# Configure the default VTK_DATA_ROOT for the location of VTKData.
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${VTK_SOURCE_DIR}/../VTKData $ENV{VTK_DATA_ROOT})

#-----------------------------------------------------------------------------
# Configure files with settings for use by the build.
CONFIGURE_FILE(
${VTK_SOURCE_DIR}/vtkConfigure.h.in 
${VTK_BINARY_DIR}/vtkConfigure.h)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/UseVTK.cmake.in 
${VTK_BINARY_DIR}/UseVTK.cmake COPYONLY IMMEDIATE)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/vtkToolkits.h.in 
${VTK_BINARY_DIR}/vtkToolkits.h)

#-----------------------------------------------------------------------------
# The entire VTK tree should use the same include path.

# Create the list of include directories needed for VTK header files.
INCLUDE(${VTK_SOURCE_DIR}/vtkIncludeDirectories.cmake)

# This should be the only INCLUDE_DIRECTORIES command in the entire
# tree, except for the CMake, Utilities, and Examples directories.  We
# need to do this in one place to make sure the order is correct.
INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIRS_BUILD_TREE}
  ${VTK_INCLUDE_DIRS_SOURCE_TREE}
  ${VTK_INCLUDE_DIRS_BUILD_TREE_CXX}
  ${VTK_INCLUDE_DIRS_SYSTEM}
)

#-----------------------------------------------------------------------------
# Help other projects use VTK.

# Install the instantiator headers.
INSTALL_FILES(/include/vtk .h vtkCommonInstantiator vtkFilteringInstantiator 
              vtkIOInstantiator vtkImagingInstantiator vtkGraphicsInstantiator)
IF(VTK_USE_RENDERING)
  INSTALL_FILES(/include/vtk .h vtkRenderingInstantiator)
ENDIF(VTK_USE_RENDERING)
IF(VTK_USE_HYBRID)
  INSTALL_FILES(/include/vtk .h vtkHybridInstantiator)
ENDIF(VTK_USE_HYBRID)
IF(VTK_USE_PATENTED)
  INSTALL_FILES(/include/vtk .h vtkPatentedInstantiator)
ENDIF(VTK_USE_PATENTED)
IF(VTK_USE_PARALLEL)
  INSTALL_FILES(/include/vtk .h vtkParallelInstantiator)
ENDIF(VTK_USE_PARALLEL)

# Install cmake extensions so user projects can load them.
INSTALL_FILES(/lib/vtk/CMake FILES
              ${VTK_SOURCE_DIR}/CMake/vtkLoadCMakeExtensions.cmake)
FOREACH(cmd VTK_WRAP_TCL2 VTK_WRAP_PYTHON2 VTK_WRAP_JAVA2
            VTK_MAKE_INSTANTIATOR2 VTK_GENERATE_JAVA_DEPENDENCIES)
  INSTALL_FILES(/lib/vtk/CMake FILES
    "${VTK_BINARY_DIR}/CMake/${CMAKE_SHARED_MODULE_PREFIX}cm${cmd}${CMAKE_SHARED_MODULE_SUFFIX}"
  )
ENDFOREACH(cmd)

# Save the compiler settings so another project can import them.
INCLUDE(${CMAKE_ROOT}/Modules/CMakeExportBuildSettings.cmake)
CMAKE_EXPORT_BUILD_SETTINGS(${VTK_BINARY_DIR}/VTKBuildSettings.cmake)
INSTALL_FILES(/lib/vtk .cmake VTKBuildSettings)

# Save library dependencies.
EXPORT_LIBRARY_DEPENDENCIES(${VTK_BINARY_DIR}/VTKLibraryDepends.cmake)

# Create the VTKConfig.cmake file containing the VTK configuration.
INCLUDE(${VTK_SOURCE_DIR}/vtkGenerateVTKConfig.cmake)

# Install some files.
INSTALL_FILES(/include/vtk .h vtkConfigure vtkToolkits)
INSTALL_FILES(/lib/vtk .cmake UseVTK VTKLibraryDepends)
IF(VTK_WRAP_HINTS)
  INSTALL_FILES(/lib/vtk FILES ${VTK_WRAP_HINTS})
ENDIF(VTK_WRAP_HINTS)

#-----------------------------------------------------------------------------
# Add compiler flags VTK needs to work on this platform.  This must be
# done AFTER the call to CMAKE_EXPORT_BUILD_SETTINGS.

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VTK_REQUIRED_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTK_REQUIRED_CXX_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${VTK_REQUIRED_EXE_LINKER_FLAGS}")

#-----------------------------------------------------------------------------
# Allow local additions to this file without CVS conflicts.
INCLUDE(${VTK_BINARY_DIR}/LocalUserOptions.cmake OPTIONAL)
INCLUDE(${VTK_SOURCE_DIR}/LocalUserOptions.cmake OPTIONAL)

# If the version of CMake was too old, complain and build nothing.
# These should be the last lines in this file.
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.5)
  MESSAGE(SEND_ERROR
          "This version of CMake is too old to build VTK.  "
          "Please upgrade to CMake 1.6.")
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.5)
