PROJECT(VTK)
INCLUDE_REGULAR_EXPRESSION("^(lex|vtk|png|j).*$")

OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
SET(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS} CACHE INTERNAL "Is this VTK built with shared libraries.")
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)
OPTION(VTK_USE_64BIT_IDS "Build VTK with 64 bit ids" OFF)

SUBDIRS(Wrapping Utilities Common Filtering Imaging Graphics IO)

SUBDIR_DEPENDS(Common Wrapping Utilities)
SUBDIR_DEPENDS(Filtering Common)
SUBDIR_DEPENDS(Imaging Filtering)
SUBDIR_DEPENDS(Graphics Filtering)
SUBDIR_DEPENDS(IO Filtering)

SET (LIBRARY_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all executables.")

SET (VTK_LIBRARY_PATH ${LIBRARY_OUTPUT_PATH} CACHE INTERNAL "Where VTK libraries are. Used by projects which depend on VTK. Do not remove.")

OPTION (VTK_USE_RENDERING "Build the rendering classes used for displaying" ON)
OPTION (VTK_DEBUG_LEAKS "Build leak checking support into vtk." OFF)
OPTION (VTK_USE_HYBRID "Build the hybrid directory classes")
OPTION (VTK_USE_PATENTED "Build the patented directory classes, these classes are patented and may require a license to use")
IF (APPLE)
  OPTION (VTK_USE_QUARTZ "Build classes for the Quartz window manager" OFF)
ENDIF(APPLE)

IF (CMAKE_HAS_X)
  OPTION(VTK_USE_X "Build classes for the X11 window manager" ON)
ENDIF (CMAKE_HAS_X)

OPTION (VTK_USE_PARALLEL "Build the parallel directory classes")
IF (VTK_USE_PARALLEL)
    OPTION (VTK_USE_MPI 
      "use MPI (Message Passing Interface) library for parallel support")
    IF(BUILD_TESTING)
     IF (DART_ROOT)
      IF (VTK_USE_MPI)
        FIND_PROGRAM(MPIDRIVER NAMES mpidriver)
        FIND_PROGRAM(MPIRUN NAMES mpirun lamexec)
        SET (MPI_PREFLAGS "-np 2" CACHE STRING " Flags used by mpi start program.")
        SET (MPI_POSTFLAGS "" CACHE STRING " Flags used by mpi start program.")
      ENDIF (VTK_USE_MPI)
     ENDIF (DART_ROOT)
    ENDIF(BUILD_TESTING)
ENDIF (VTK_USE_PARALLEL)

OPTION (VTK_MANGLE_MESA "Use mangled Mesa with OpenGL" OFF)
IF (VTK_MANGLE_MESA)
  FIND_PATH(MESA_MANGLE_PATH gl_mangle.h )
  FIND_LIBRARY(MESA_LIBRARY MesaGL
     /usr/local/lib 
  )
  IF(MESA_MANGLE_PATH)
    USE_MANGLED_MESA(${MESA_MANGLE_PATH} ${VTK_BINARY_DIR}/MangleMesaInclude)
  ENDIF(MESA_MANGLE_PATH)
ENDIF(VTK_MANGLE_MESA)

OPTION(VTK_WRAP_TCL "wrap classes into the TCL intepreted language")
IF (VTK_WRAP_TCL)
  INCLUDE (${CMAKE_ROOT}/Modules/FindTCL.cmake)
  UTILITY_SOURCE(VTK_WRAP_TCL_EXE vtkWrapTcl Wrapping vtkWrapTcl.c)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  IF (EXECUTABLE_OUTPUT_PATH)
    SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
  ELSE (EXECUTABLE_OUTPUT_PATH)
    SET (VTK_EXECUTABLE ${VTK_BINARY_DIR}/Wrapping/Tcl/${CMAKE_CFG_INTDIR}/vtk)
  ENDIF (EXECUTABLE_OUTPUT_PATH)
ENDIF (VTK_WRAP_TCL)

OPTION(VTK_WRAP_PYTHON "wrap classes into the Python interpreted language")
IF (VTK_WRAP_PYTHON)
  INCLUDE (${CMAKE_ROOT}/Modules/FindPythonLibs.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  UTILITY_SOURCE(VTK_WRAP_PYTHON_EXE vtkWrapPython Wrapping vtkWrapPython.c)
ENDIF (VTK_WRAP_PYTHON)

IF(BUILD_TESTING)
 IF (DART_ROOT)
  FIND_PROGRAM(PYTHON_EXECUTABLE NAMES python)
 ENDIF (DART_ROOT)
ENDIF(BUILD_TESTING)

OPTION(VTK_WRAP_JAVA "wrap classes into the Java language")
IF (VTK_WRAP_JAVA)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJNI.cmake)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJNI.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  UTILITY_SOURCE(VTK_WRAP_JAVA_EXE vtkWrapJava Wrapping vtkWrapJava.c)
  UTILITY_SOURCE(VTK_PARSE_JAVA_EXE vtkParseJava Wrapping vtkParseJava.c)
  # where to write the resulting .java files
  IF (NOT VTK_JAVA_HOME)
    SET (VTK_JAVA_HOME ${VTK_BINARY_DIR}/java/vtk CACHE PATH "Path to Java install")
  ENDIF (NOT VTK_JAVA_HOME)
ENDIF (VTK_WRAP_JAVA)

OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library", OFF)

#
# try to find the data files for VTKData
#
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${VTK_SOURCE_DIR}/../VTKData $ENV{VTK_DATA_ROOT})

# where will executable tests be written ?
IF (EXECUTABLE_OUTPUT_PATH)
    SET (CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})
ELSE (EXECUTABLE_OUTPUT_PATH)
    SET (CXX_TEST_PATH .)
ENDIF (EXECUTABLE_OUTPUT_PATH)

INCLUDE (${VTK_SOURCE_DIR}/vtkCMakeOptions.cmake)

IF (VTK_USE_RENDERING)
  OPTION(VTK_USE_VOLUMEPRO "Build VTK with volume pro support" OFF)
  IF (VTK_USE_VOLUMEPRO)
    FIND_LIBRARY(VLI_LIBRARY
      NAMES vli
      PATHS  
        /usr/lib 
        /usr/local/lib
        "C:/Program Files/VolumePro/lib" 
      )
    FIND_PATH(VLI_INCLUDE_PATH vli.h 
        /usr/include 
        /usr/local/include
        "C:/Program Files/VolumePro/inc" 
        )
  ENDIF (VTK_USE_VOLUMEPRO)
  INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)

  FIND_PATH(OSMESA_INCLUDE_PATH GL/osmesa.h 
  /usr/include 
  /usr/local/include 
  /usr/openwin/share/include 
  /opt/graphics/OpenGL/include 
  /usr/X11R6/include 
  )

  FIND_LIBRARY(MESA_OS_LIBRARY OSMesa
  /usr/lib 
  /usr/local/lib 
  /opt/graphics/OpenGL/lib 
  /usr/openwin/lib 
  /usr/X11R6/lib
  )
  IF(UNIX)
    OPTION(VTK_OPENGL_HAS_OSMESA "The opengl library being used supports off screen Mesa calls." OFF)
  ENDIF(UNIX)
  SUBDIRS(Rendering)
  SUBDIR_DEPENDS(Rendering Imaging Graphics IO)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_HYBRID)
  IF (WIN32)
    OPTION(VTK_USE_VIDEO_FOR_WINDOWS "Use Video for Windows for video input" ON)
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
  SUBDIR_DEPENDS(Hybrid Rendering)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PATENTED)
  SUBDIRS(Patented)
  SUBDIR_DEPENDS(Patented Rendering)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_PARALLEL)
  SUBDIRS(Parallel)
  SUBDIR_DEPENDS(Parallel Rendering Hybrid)
  IF (VTK_USE_PATENTED)
    IF (VTK_USE_MPI)
      IF (VTK_WRAP_TCL)
         SUBDIR_DEPENDS(Parallel Patented)
      ENDIF (VTK_WRAP_TCL)
    ENDIF (VTK_USE_MPI)
  ENDIF (VTK_USE_PATENTED)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_WRAP_TCL)
  SUBDIRS(Wrapping/Tcl)
  SUBDIR_DEPENDS(Wrapping/Tcl Imaging Graphics IO)
  IF (VTK_USE_RENDERING)
    SUBDIR_DEPENDS(Wrapping/Tcl Rendering)
  ENDIF (VTK_USE_RENDERING)
  IF (VTK_USE_HYBRID)
    SUBDIR_DEPENDS(Wrapping/Tcl Hybrid)
  ENDIF (VTK_USE_HYBRID)  
  IF (VTK_USE_PATENTED)
    SUBDIR_DEPENDS(Wrapping/Tcl Patented)
  ENDIF (VTK_USE_PATENTED)
  IF (VTK_USE_PARALLEL)
    SUBDIR_DEPENDS(Wrapping/Tcl Parallel)
  ENDIF (VTK_USE_PARALLEL)
ENDIF (VTK_WRAP_TCL)

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
    SUBDIRS(Common/Testing Filtering/Testing Graphics/Testing 
            Imaging/Testing IO/Testing)

    SUBDIR_DEPENDS(Common/Testing Common)
    SUBDIR_DEPENDS(Filtering/Testing Filtering)
    SUBDIR_DEPENDS(Graphics/Testing Graphics)
    SUBDIR_DEPENDS(Imaging/Testing Imaging)
    SUBDIR_DEPENDS(IO/Testing IO)

    IF (VTK_USE_RENDERING)
      SUBDIRS(Rendering/Testing)
      SUBDIR_DEPENDS(Rendering/Testing Rendering)
      SUBDIR_DEPENDS(Graphics/Testing Rendering)
    ENDIF(VTK_USE_RENDERING)
    IF (VTK_USE_HYBRID)
      SUBDIRS(Hybrid/Testing)
      SUBDIR_DEPENDS(Hybrid/Testing Hybrid)
    ENDIF(VTK_USE_HYBRID)
    IF (VTK_USE_PARALLEL)
      SUBDIRS(Parallel/Testing)
      SUBDIR_DEPENDS(Parallel/Testing Parallel)
    ENDIF(VTK_USE_PARALLEL)
    IF (VTK_USE_PATENTED)
      SUBDIRS(Patented/Testing)
      SUBDIR_DEPENDS(Patented/Testing Patented)
    ENDIF(VTK_USE_PATENTED)
ENDIF(BUILD_TESTING)

# Add the examples
IF (BUILD_EXAMPLES)
  SUBDIRS(Examples)
  SUBDIR_DEPENDS(Examples Imaging Graphics IO)
  IF (VTK_USE_RENDERING)
    SUBDIR_DEPENDS(Examples Rendering)
  ENDIF (VTK_USE_RENDERING)
  IF (VTK_USE_HYBRID)
    SUBDIR_DEPENDS(Examples Hybrid)
  ENDIF (VTK_USE_HYBRID)  
  IF (VTK_USE_PATENTED)
    SUBDIR_DEPENDS(Examples Patented)
  ENDIF (VTK_USE_PATENTED)
  IF (VTK_USE_PARALLEL)
    SUBDIR_DEPENDS(Examples Parallel)
  ENDIF (VTK_USE_PARALLEL)
ENDIF (BUILD_EXAMPLES)

#
INSTALL_FILES(/include/vtk .h vtkConfigure vtkToolkits)
INSTALL_FILES(/include/vtk .cmake UseVTK)
INSTALL_FILES(/include/vtk .txt CMakeCache)

INCLUDE (${VTK_BINARY_DIR}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/LocalUserOptions.cmake OPTIONAL)









