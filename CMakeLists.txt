PROJECT(VTK)
INCLUDE_REGULAR_EXPRESSION("^(lex|vtk|png|j).*$")

OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
SET(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS} CACHE INTERNAL "Is this VTK built with shared libraries.")
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)
OPTION(VTK_USE_64BIT_IDS "Build VTK with 64 bit ids" OFF)
MARK_AS_ADVANCED(VTK_USE_64BIT_IDS)

SUBDIRS(Wrapping Utilities Common Filtering Imaging Graphics IO)

SET (LIBRARY_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all executables.")
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

SET (VTK_LIBRARY_PATH ${LIBRARY_OUTPUT_PATH} CACHE INTERNAL "Where VTK libraries are. Used by projects which depend on VTK. Do not remove.")

OPTION (VTK_USE_RENDERING "Build the rendering classes used for displaying" ON)
OPTION (VTK_DEBUG_LEAKS "Build leak checking support into vtk." OFF)
MARK_AS_ADVANCED(VTK_DEBUG_LEAKS)
OPTION (VTK_USE_HYBRID "Build the hybrid directory classes")
OPTION (VTK_USE_PATENTED "Build the patented directory classes, these classes are patented and may require a license to use")
IF (APPLE)
  OPTION (VTK_USE_QUARTZ "Build classes for the Quartz window manager" ON)
  IF (CMAKE_HAS_X)
    OPTION(VTK_USE_X "Build classes for the X11 window manager" OFF)
  ENDIF (CMAKE_HAS_X)
ELSE(APPLE)
  IF (CMAKE_HAS_X)
    SET(VTK_USE_X ON)
  ENDIF (CMAKE_HAS_X)
ENDIF(APPLE)


OPTION (VTK_USE_PARALLEL "Build the parallel directory classes")
IF (VTK_USE_PARALLEL)
    OPTION (VTK_USE_MPI 
      "use MPI (Message Passing Interface) library for parallel support")
    MARK_AS_ADVANCED(VTK_USE_MPI)
    IF(BUILD_TESTING)
      IF (VTK_USE_MPI)
        FIND_PROGRAM(MPIRUN NAMES mpirun lamexec)
        MARK_AS_ADVANCED(MPIRUN)
        SET (MPI_PREFLAGS "-np 2" CACHE STRING " Flags used by mpi start program.")
        SET (MPI_POSTFLAGS "" CACHE STRING " Flags used by mpi start program.")
        MARK_AS_ADVANCED(MPI_PREFLAGS MPI_POSTFLAGS)
      ENDIF (VTK_USE_MPI)
    ENDIF(BUILD_TESTING)
ENDIF (VTK_USE_PARALLEL)

OPTION (VTK_MANGLE_MESA "Use mangled Mesa with OpenGL" OFF)
MARK_AS_ADVANCED(VTK_MANGLE_MESA)
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
  MARK_AS_ADVANCED(VTK_WRAP_TCL_EXE)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  MARK_AS_ADVANCED(VTK_WRAP_HINTS)
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
  MARK_AS_ADVANCED(VTK_WRAP_HINTS)
  UTILITY_SOURCE(VTK_WRAP_PYTHON_EXE vtkWrapPython Wrapping vtkWrapPython.c)
  MARK_AS_ADVANCED(VTK_WRAP_PYTHON_EXE)
  IF (WIN32)
    IF (NOT VTK_BUILD_SHARED_LIBS)
      MESSAGE("Python support requires BUILD_SHARED_LIBS to be on, please turn it on." "Error")
    ENDIF (NOT VTK_BUILD_SHARED_LIBS)  
  ENDIF (WIN32)
ENDIF (VTK_WRAP_PYTHON)

IF(BUILD_TESTING AND VTK_WRAP_PYTHON)
 IF (DART_ROOT)
  FIND_PROGRAM(PYTHON_EXECUTABLE NAMES python)
  MARK_AS_ADVANCED(PYTHON_EXECUTABLE)
 ENDIF (DART_ROOT)
ENDIF(BUILD_TESTING AND VTK_WRAP_PYTHON)

OPTION(VTK_WRAP_JAVA "wrap classes into the Java language")
IF (VTK_WRAP_JAVA)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJNI.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  MARK_AS_ADVANCED(VTK_WRAP_HINTS)
  UTILITY_SOURCE(VTK_WRAP_JAVA_EXE vtkWrapJava Wrapping vtkWrapJava.c)
  UTILITY_SOURCE(VTK_PARSE_JAVA_EXE vtkParseJava Wrapping vtkParseJava.c)
  MARK_AS_ADVANCED(VTK_WRAP_JAVA_EXE VTK_PARSE_JAVA_EXE)
  # where to write the resulting .java files
  IF (NOT VTK_JAVA_HOME)
    SET (VTK_JAVA_HOME ${VTK_BINARY_DIR}/java/vtk CACHE PATH "Path to Java install")
  ENDIF (NOT VTK_JAVA_HOME)
  IF (WIN32)
    IF (NOT VTK_BUILD_SHARED_LIBS)
      MESSAGE("Java support requires BUILD_SHARED_LIBS to be on, please turn it on." "Error")
    ENDIF (NOT VTK_BUILD_SHARED_LIBS)  
  ENDIF (WIN32)
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
    OPTION(VTK_OPENGL_HAS_OSMESA "The opengl library being used supports off screen Mesa calls." OFF)
    MARK_AS_ADVANCED(VTK_OPENGL_HAS_OSMESA)
  ENDIF(UNIX)

  IF (VTK_OPENGL_HAS_OSMESA OR VTK_MANGLE_MESA)
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
  ENDIF (VTK_OPENGL_HAS_OSMESA OR VTK_MANGLE_MESA)

  SUBDIRS(Rendering)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_HYBRID)
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
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PATENTED)
  SUBDIRS(Patented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_PARALLEL)
  SUBDIRS(Parallel)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_WRAP_TCL)
  SUBDIRS(Wrapping/Tcl)
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

#
INSTALL_FILES(/include/vtk .h vtkConfigure vtkToolkits)
INSTALL_FILES(/include/vtk .cmake UseVTK)
INSTALL_FILES(/include/vtk .txt CMakeCache)

INCLUDE (${VTK_BINARY_DIR}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/LocalUserOptions.cmake OPTIONAL)










