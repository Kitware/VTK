CMAKE_MINIMUM_REQUIRED(VERSION 1.4)
PROJECT(VTK)
INCLUDE_REGULAR_EXPRESSION("^(lex|vtk|png|j).*$")

#-----------------------------------------------------------------------------
# VTK version number.  An even minor number corresponds to releases.
SET(VTK_MAJOR_VERSION 4)
SET(VTK_MINOR_VERSION 1)
SET(VTK_BUILD_VERSION 1)
SET(VTK_VERSION
    "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")

#-----------------------------------------------------------------------------
# Figure out which version of the VTK_MAKE_INSTANTIATOR command is
# available with this version of CMake.
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
   IF (COMMAND VTK_MAKE_INSTANTIATOR2)
   ELSE (COMMAND VTK_MAKE_INSTANTIATOR2)
      MESSAGE(STATUS "Compiling VTK loaded commands")
      TRY_COMPILE(COMPILE_OK 
         ${VTK_BINARY_DIR}/CMake 
         ${VTK_SOURCE_DIR}/CMake 
         VTK_LOADED_COMMANDS)
      IF (COMPILE_OK)
         MESSAGE(STATUS "Compiling VTK loaded commands - done")
         # load the four extra CMake commands 
         LOAD_COMMAND(VTK_WRAP_TCL2 
            ${VTK_BINARY_DIR}/CMake ${VTK_BINARY_DIR}/CMake/Debug)
         LOAD_COMMAND(VTK_WRAP_PYTHON2 
            ${VTK_BINARY_DIR}/CMake ${VTK_BINARY_DIR}/CMake/Debug)
         LOAD_COMMAND(VTK_WRAP_JAVA2 
            ${VTK_BINARY_DIR}/CMake ${VTK_BINARY_DIR}/CMake/Debug)
         LOAD_COMMAND(VTK_MAKE_INSTANTIATOR2 
            ${VTK_BINARY_DIR}/CMake ${VTK_BINARY_DIR}/CMake/Debug)
      ELSE (COMPILE_OK)
         MESSAGE(STATUS "Compiling VTK loaded commands - failed")
         MESSAGE("failed to compile VTK extensions to CMake")
      ENDIF (COMPILE_OK)
   ENDIF (COMMAND VTK_MAKE_INSTANTIATOR2)
   SET(VTK_USE_INSTANTIATOR_NEW "1")

  # Install cmake extension directory so user projects can build the commands.
  INSTALL_FILES(/lib/vtk/CMake FILES
                ${VTK_SOURCE_DIR}/CMake/CMakeLists.txt
                ${VTK_SOURCE_DIR}/CMake/cmVTKWrapTcl2Command.c
                ${VTK_SOURCE_DIR}/CMake/cmVTKWrapPython2Command.c
                ${VTK_SOURCE_DIR}/CMake/cmVTKWrapJava2Command.c
                ${VTK_SOURCE_DIR}/CMake/cmVTKMakeInstantiator2Command.c)
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)

#-----------------------------------------------------------------------------
OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
SET(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS} CACHE INTERNAL "Is this VTK built with shared libraries.")
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)
OPTION(VTK_USE_64BIT_IDS "Build VTK with 64 bit ids" OFF)
MARK_AS_ADVANCED(VTK_USE_64BIT_IDS)

SUBDIRS(Wrapping Utilities Common Filtering Imaging Graphics IO)
INSTALL_FILES(/include/vtk .h vtkCommonInstantiator vtkFilteringInstantiator 
  vtkIOInstantiator vtkImagingInstantiator vtkGraphicsInstantiator)

SET (LIBRARY_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all executables.")
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

SET (VTK_LIBRARY_PATH ${LIBRARY_OUTPUT_PATH} CACHE INTERNAL "Where VTK libraries are. Used by projects which depend on VTK. Do not remove.")

SET (VTK_EXECUTABLE_PATH ${EXECUTABLE_OUTPUT_PATH} CACHE INTERNAL "Where VTK executables are. Used by projects which depend on VTK. Do not remove.")

OPTION (VTK_USE_RENDERING "Build the rendering classes used for displaying" ON)
OPTION (VTK_DEBUG_LEAKS "Build leak checking support into vtk." OFF)
MARK_AS_ADVANCED(VTK_DEBUG_LEAKS)
OPTION (VTK_USE_HYBRID "Build the hybrid directory classes")
OPTION (VTK_USE_PATENTED "Build the patented directory classes, these classes are patented and may require a license to use")
IF (APPLE)
  OPTION (VTK_USE_COCOA "Build classes using Cocoa API - Can only use one of these!" OFF)
  OPTION (VTK_USE_CARBON "Build classes using Carbon API" ON)
  IF (CMAKE_HAS_X)
    OPTION(VTK_USE_X "Build classes for the X11 window manager" OFF)
  ENDIF (CMAKE_HAS_X)
ELSE(APPLE)
  IF (CMAKE_HAS_X)
    OPTION(VTK_USE_X "Build classes for the X11 window manager" ON)
  ENDIF (CMAKE_HAS_X)
ENDIF(APPLE)


OPTION (VTK_USE_PARALLEL "Build the parallel directory classes")
IF (VTK_USE_PARALLEL)
    OPTION (VTK_USE_MPI 
      "use MPI (Message Passing Interface) library for parallel support")
    MARK_AS_ADVANCED(VTK_USE_MPI)
    IF(BUILD_TESTING)
      IF (VTK_USE_MPI)
        FIND_PROGRAM(VTK_MPIRUN_EXE NAMES mpirun lamexec)
        MARK_AS_ADVANCED(VTK_MPIRUN_EXE)
        SET (VTK_MPI_PREFLAGS "-np 2" CACHE STRING " Flags used by mpi start program.")
        SET (VTK_MPI_POSTFLAGS "" CACHE STRING "Flags used by mpi start program.")
        SET (VTK_MPI_MAX_NUMPROCS "2" CACHE STRING "Maximum number of processors available to run parallel applications.")
        MARK_AS_ADVANCED(VTK_MPI_PREFLAGS VTK_MPI_POSTFLAGS VTK_MPI_MAX_NUMPROCS)
        SEPARATE_ARGUMENTS(VTK_MPI_PREFLAGS)
        SEPARATE_ARGUMENTS(VTK_MPI_POSTFLAGS)
      ENDIF (VTK_USE_MPI)
    ENDIF(BUILD_TESTING)
ENDIF (VTK_USE_PARALLEL)

OPTION (VTK_USE_MANGLED_MESA "Use mangled Mesa with OpenGL" OFF)
MARK_AS_ADVANCED(VTK_USE_MANGLED_MESA)
IF (VTK_USE_MANGLED_MESA)
  FIND_PATH(MANGLED_MESA_INCLUDE_DIR GL/gl_mangle.h )
  FIND_LIBRARY(MANGLED_MESA_LIBRARY MesaGL)
  IF(MANGLED_MESA_INCLUDE_DIR)
    USE_MANGLED_MESA(${MANGLED_MESA_INCLUDE_DIR}/GL
                     ${VTK_BINARY_DIR}/MangleMesaInclude)
  ENDIF(MANGLED_MESA_INCLUDE_DIR)
ENDIF(VTK_USE_MANGLED_MESA)

OPTION(VTK_WRAP_TCL "wrap classes into the TCL intepreted language")
IF (VTK_WRAP_TCL)
  INCLUDE (${CMAKE_ROOT}/Modules/FindTCL.cmake)
  UTILITY_SOURCE(VTK_WRAP_TCL_EXE vtkWrapTcl Wrapping vtkWrapTcl.c)
  MARK_AS_ADVANCED(VTK_WRAP_TCL_EXE)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  MARK_AS_ADVANCED(VTK_WRAP_HINTS)
  SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
  IF(VTK_USE_PARALLEL)
    IF(VTK_USE_MPI)
      SET (PVTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/pvtk)
    ENDIF(VTK_USE_MPI)
  ENDIF(VTK_USE_PARALLEL)
ENDIF (VTK_WRAP_TCL)

OPTION(VTK_WRAP_PYTHON "wrap classes into the Python interpreted language")
IF (VTK_WRAP_PYTHON)
  INCLUDE (${CMAKE_ROOT}/Modules/FindPythonLibs.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  MARK_AS_ADVANCED(VTK_WRAP_HINTS)
  UTILITY_SOURCE(VTK_WRAP_PYTHON_EXE vtkWrapPython Wrapping vtkWrapPython.c)
  MARK_AS_ADVANCED(VTK_WRAP_PYTHON_EXE)
  IF (WIN32)
    IF (NOT VTK_BUILD_SHARED_LIBS)
      MESSAGE("Python support requires BUILD_SHARED_LIBS to be on, please turn it on." "Error")
    ENDIF (NOT VTK_BUILD_SHARED_LIBS)  
  ENDIF (WIN32)
ENDIF (VTK_WRAP_PYTHON)

# python executable is used by some tests
# whether VTK_WRAP_PYTHON is on or not.
# do not add a VTK_WRAP_PYTHON to this if.
IF(BUILD_TESTING)
 IF (DART_ROOT)
  FIND_PROGRAM(PYTHON_EXECUTABLE NAMES python2.2 python2.1 python1.6 python1.5 python)
  MARK_AS_ADVANCED(PYTHON_EXECUTABLE)
 ENDIF (DART_ROOT)
ENDIF(BUILD_TESTING)

OPTION(VTK_WRAP_JAVA "wrap classes into the Java language")
IF (VTK_WRAP_JAVA)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJNI.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  MARK_AS_ADVANCED(VTK_WRAP_HINTS)
  UTILITY_SOURCE(VTK_WRAP_JAVA_EXE vtkWrapJava Wrapping vtkWrapJava.c)
  UTILITY_SOURCE(VTK_PARSE_JAVA_EXE vtkParseJava Wrapping vtkParseJava.c)
  MARK_AS_ADVANCED(VTK_WRAP_JAVA_EXE VTK_PARSE_JAVA_EXE)
  # where to write the resulting .java files
  SET (VTK_JAVA_HOME ${VTK_BINARY_DIR}/java/vtk)
  MAKE_DIRECTORY(${VTK_JAVA_HOME})
  IF (WIN32)
    IF (NOT VTK_BUILD_SHARED_LIBS)
      MESSAGE("Java support requires BUILD_SHARED_LIBS to be on, please turn it on." "Error")
    ENDIF (NOT VTK_BUILD_SHARED_LIBS)  
  ENDIF (WIN32)
ENDIF (VTK_WRAP_JAVA)

IF(WIN32)
  IF(UNIX) 
    SET(VTK_DISPLAY_WIN32_WARNINGS ON)
  ELSE(UNIX)
    OPTION(VTK_DISPLAY_WIN32_WARNINGS "Show all warnings on Visual C++ with option /W4"
           OFF)
    MARK_AS_ADVANCED(VTK_DISPLAY_WIN32_WARNINGS)
  ENDIF(UNIX)   
ENDIF(WIN32)

IF(CMAKE_COMPILER_IS_GNUCXX)
  OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library" ON)
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_BUILD_TOOL MATCHES "devenv")
    OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library" ON)
  ELSE(CMAKE_BUILD_TOOL MATCHES "devenv")
    OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library" OFF)
  ENDIF(CMAKE_BUILD_TOOL MATCHES "devenv")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

#
# try to find the data files for VTKData
#
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${VTK_SOURCE_DIR}/../VTKData $ENV{VTK_DATA_ROOT})

# where will executable tests be written ?
SET (CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

IF (VTK_USE_RENDERING)
  OPTION(VTK_USE_VOLUMEPRO "Build VTK with volume pro support" OFF)
  IF (VTK_USE_VOLUMEPRO)
    FIND_LIBRARY(VLI_LIBRARY_FOR_VG500
      NAMES vli
      PATHS  
        /usr/lib 
        /usr/local/lib
        /opt/vli/lib
        "C:/Program Files/VolumePro/lib"
      )
    FIND_LIBRARY(VLI_LIBRARY_FOR_VP1000
      NAMES vli3
      PATHS
        /usr/lib
        /usr/local/lib
        /opt/vli/lib
        "C:/Program Files/VolumePro1000/lib"
      )
    IF (VLI_LIBRARY_FOR_VG500)
      IF (WIN32)
        FIND_PATH(VLI_INCLUDE_PATH_FOR_VG500 VolumePro/inc/vli.h 
          "C:/Program Files"
          )
      ELSE (WIN32)
        FIND_PATH(VLI_INCLUDE_PATH_FOR_VG500 vli/include/vli.h
          /usr
          /usr/local
          /opt
          )
      ENDIF (WIN32)
      IF (VLI_INCLUDE_PATH_FOR_VG500)
        SET (VTK_HAVE_VG500 ON)
      ENDIF (VLI_INCLUDE_PATH_FOR_VG500)
    ENDIF (VLI_LIBRARY_FOR_VG500)
    IF (VLI_LIBRARY_FOR_VP1000)
      IF (WIN32)
        FIND_PATH(VLI_INCLUDE_PATH_FOR_VP1000 VolumePro1000/inc/vli.h 
          "C:/Program Files"
          )
      ELSE (WIN32)
        FIND_PATH(VLI_INCLUDE_PATH_FOR_VP1000 vli3/include/vli.h
          /usr
          /usr/local
          /opt/vli/include
          )
      ENDIF (WIN32)
      IF (VLI_INCLUDE_PATH_FOR_VP1000)
        SET (VTK_HAVE_VP1000 ON)
      ENDIF (VLI_INCLUDE_PATH_FOR_VP1000)
    ENDIF (VLI_LIBRARY_FOR_VP1000)
  ENDIF (VTK_USE_VOLUMEPRO)

  INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)

  IF(UNIX)
    OPTION(VTK_OPENGL_HAS_OSMESA 
           "The opengl library being used supports off screen Mesa calls." 
           OFF)
    MARK_AS_ADVANCED(VTK_OPENGL_HAS_OSMESA)
  ENDIF(UNIX)

  IF (VTK_OPENGL_HAS_OSMESA)
    FIND_PATH(OSMESA_INCLUDE_DIR GL/osmesa.h 
      /usr/include 
      /usr/local/include 
      /usr/openwin/share/include 
      /opt/graphics/OpenGL/include 
      /usr/X11R6/include 
    )

    FIND_LIBRARY(OSMESA_LIBRARY OSMesa
      /usr/lib 
      /usr/local/lib 
      /opt/graphics/OpenGL/lib 
      /usr/openwin/lib 
      /usr/X11R6/lib
    )
  ENDIF (VTK_OPENGL_HAS_OSMESA)

  IF (VTK_USE_MANGLED_MESA)
    FIND_LIBRARY(MANGLED_OSMESA_LIBRARY OSMesa)
  ENDIF (VTK_USE_MANGLED_MESA)

  SUBDIRS(Rendering)
  INSTALL_FILES(/include/vtk .h vtkRenderingInstantiator)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_HYBRID)
  # hybrid requires rendering
  IF (NOT VTK_USE_RENDERING)
    MESSAGE("The Hybrid option requires Rendering, but you do not have Rendering selected." "Warning")
  ENDIF (NOT VTK_USE_RENDERING)

  IF (WIN32)
    OPTION(VTK_USE_VIDEO_FOR_WINDOWS "Use Video for Windows for video input" OFF)
    OPTION(VTK_USE_MATROX_IMAGING "Use Matrox Imaging Library for video input" OFF)
    IF (VTK_USE_MATROX_IMAGING)
      FIND_LIBRARY(MIL_LIBRARY MIL
        "C:/Program Files/Matrox Imaging/mil/library/winnt/msc/dll"
        "C:/Program Files/Matrox Imaging/mil/library/windows/msc/dll"
        )
      FIND_PATH(MIL_INCLUDE_PATH mil.h
        "C:/Program Files/Matrox Imaging/mil/include"
        )
    ENDIF (VTK_USE_MATROX_IMAGING)
  ENDIF (WIN32)

  SUBDIRS(Hybrid)
  INSTALL_FILES(/include/vtk .h vtkHybridInstantiator)
ENDIF (VTK_USE_HYBRID)

INCLUDE (${VTK_SOURCE_DIR}/vtkCMakeOptions.cmake)

IF (VTK_USE_PATENTED)
  SUBDIRS(Patented)
  INSTALL_FILES(/include/vtk .h vtkPatentedInstantiator)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_PARALLEL)
  SUBDIRS(Parallel)
  INSTALL_FILES(/include/vtk .h vtkParallelInstantiator)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_WRAP_TCL)
  SUBDIRS(Wrapping/Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Wrapping/Python)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJava.cmake)
  SUBDIRS(Wrapping/Java)
ENDIF (VTK_WRAP_JAVA)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/vtkConfigure.h.in 
${VTK_BINARY_DIR}/vtkConfigure.h)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/UseVTK.cmake.in 
${VTK_BINARY_DIR}/UseVTK.cmake COPYONLY IMMEDIATE)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/vtkToolkits.h.in 
${VTK_BINARY_DIR}/vtkToolkits.h)

# Include the standard Dart testing module
INCLUDE (${CMAKE_ROOT}/Modules/Dart.cmake)

# Add the testing directories
IF(BUILD_TESTING)
    ENABLE_TESTING()
    SUBDIRS(
      Common/Testing 
      Filtering/Testing 
      Graphics/Testing 
      Imaging/Testing 
      IO/Testing
#      Examples/Build
    )

    IF (VTK_USE_RENDERING)
      SUBDIRS(Rendering/Testing)
    ENDIF(VTK_USE_RENDERING)
    IF (VTK_USE_HYBRID)
      SUBDIRS(Hybrid/Testing)
    ENDIF(VTK_USE_HYBRID)
    IF (VTK_USE_PARALLEL)
      SUBDIRS(Parallel/Testing)
    ENDIF(VTK_USE_PARALLEL)
    IF (VTK_USE_PATENTED)
      SUBDIRS(Patented/Testing)
    ENDIF(VTK_USE_PATENTED)
ENDIF(BUILD_TESTING)

# Add the examples
IF (BUILD_EXAMPLES)
  SUBDIRS(Examples)
ENDIF (BUILD_EXAMPLES)

INSTALL_FILES(/include/vtk .h vtkConfigure vtkToolkits)
INSTALL_FILES(/lib/vtk .cmake UseVTK)

# Hack for cmake 1.4 module.  It needs the file here.
INSTALL_FILES(/include/vtk .cmake UseVTK)

INCLUDE (${VTK_BINARY_DIR}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/LocalUserOptions.cmake OPTIONAL)

INCLUDE (${VTK_SOURCE_DIR}/GenerateVTKConfig.cmake)
