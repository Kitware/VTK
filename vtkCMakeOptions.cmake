#
# This file can be included by other projects that use or depend on VTK
# it sets up many default parameters and include paths
#

SET (LIBRARY_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all executables.")

OPTION (VTK_USE_RENDERING "Build the rendering classes used for displaying" ON)
IF (VTK_USE_RENDERING)
  INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Rendering)
  LINK_DIRECTORIES(${VTK_BINARY_DIR}/Rendering) 
ENDIF (VTK_USE_RENDERING)

OPTION (VTK_DEBUG_LEAKS "Build leak checking support into vtk." OFF)

OPTION (VTK_USE_HYBRID "Build the hybrid directory classes")
IF (VTK_USE_HYBRID)
  # hybrid requires rendering
  IF (VTK_USE_RENDERING)
    INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Hybrid)
    LINK_DIRECTORIES(${VTK_BINARY_DIR}/Hybrid) 
  ELSE (VTK_USE_RENDERING)
    MESSAGE("The Hybrid option requires Rendering, but you do not have Rendering selected." "Warning")
  ENDIF (VTK_USE_RENDERING)
ENDIF (VTK_USE_HYBRID)

OPTION (VTK_USE_PATENTED "Build the patented directory classes, these classes are patented and may require a license to use")
IF (VTK_USE_PATENTED)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Patented)
  LINK_DIRECTORIES(${VTK_BINARY_DIR}/Patented) 
ENDIF (VTK_USE_PATENTED)

OPTION (VTK_USE_PARALLEL "Build the parallel directory classes")
IF (VTK_USE_PARALLEL)
    OPTION (VTK_USE_MPI 
      "use MPI (Message Passing Interface) library for parallel support")
    INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Parallel)
    LINK_DIRECTORIES(${VTK_BINARY_DIR}/Parallel) 
    IF(BUILD_TESTING)
     IF (DART_ROOT)
      FIND_PROGRAM(MPIRUN names mpirun lamexec)
      SET (MPI_PREFLAGS "-np 2" CACHE STRING " Flags used by mpi start program.")
      SET (MPI_POSTFLAGS "" CACHE STRING " Flags used by mpi start program.")
     ENDIF (DART_ROOT)
    ENDIF(BUILD_TESTING)
ENDIF (VTK_USE_PARALLEL)

#
# get information for Tcl wrapping 
#
OPTION(VTK_WRAP_TCL "wrap classes into the TCL intepreted language")
IF (VTK_WRAP_TCL)
  INCLUDE (${CMAKE_ROOT}/Modules/FindTCL.cmake)

  # add in the Tcl values if found
  IF (TCL_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
  ENDIF (TCL_INCLUDE_PATH)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  UTILITY_SOURCE(VTK_WRAP_TCL_EXE vtkWrapTcl Wrapping vtkWrapTcl.c)
  IF (EXECUTABLE_OUTPUT_PATH)
    SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
  ELSE (EXECUTABLE_OUTPUT_PATH)
    SET (VTK_EXECUTABLE ${VTK_BINARY_DIR}/Wrapping/Tcl/vtk)
  ENDIF (EXECUTABLE_OUTPUT_PATH)
ENDIF (VTK_WRAP_TCL)

#
# get information for Python wrapping 
#
OPTION(VTK_WRAP_PYTHON "wrap classes into the Python interpreted language")
IF (VTK_WRAP_PYTHON)
  INCLUDE (${CMAKE_ROOT}/Modules/FindPythonLibs.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  UTILITY_SOURCE(VTK_WRAP_PYTHON_EXE vtkWrapPython Wrapping vtkWrapPython.c)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  IF(BUILD_TESTING)
   IF (DART_ROOT)
    FIND_PROGRAM(PYTHON_EXECUTABLE names python)
   ENDIF (DART_ROOT)
  ENDIF(BUILD_TESTING)

ENDIF (VTK_WRAP_PYTHON)

#
# get information for Java wrapping 
#
OPTION(VTK_WRAP_JAVA "wrap classes into the Java language")
IF (VTK_WRAP_JAVA)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJNI.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  UTILITY_SOURCE(VTK_WRAP_JAVA_EXE vtkWrapJava Wrapping vtkWrapJava.c)
  UTILITY_SOURCE(VTK_PARSE_JAVA_EXE vtkParseJava Wrapping vtkParseJava.c)
  IF (JAVA_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH})
  ENDIF (JAVA_INCLUDE_PATH)
  IF (JAVA_INCLUDE_PATH2)
    INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH2})
  ENDIF (JAVA_INCLUDE_PATH2)
  IF (JAVA_AWT_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${JAVA_AWT_INCLUDE_PATH})
  ENDIF (JAVA_AWT_INCLUDE_PATH)
  # where to write the resulting .java files
  IF (NOT VTK_JAVA_HOME)
    SET (VTK_JAVA_HOME . CACHE PATH "Path to Java install")
  ENDIF (NOT VTK_JAVA_HOME)
ENDIF (VTK_WRAP_JAVA)

INCLUDE_DIRECTORIES(
${VTK_BINARY_DIR} 
${VTK_SOURCE_DIR}/Common
${VTK_SOURCE_DIR}/Filtering
${VTK_SOURCE_DIR}/Imaging
${VTK_SOURCE_DIR}/Graphics
${VTK_SOURCE_DIR}/IO
)

LINK_DIRECTORIES(
${VTK_BINARY_DIR}/Common 
${VTK_BINARY_DIR}/Filtering
${VTK_BINARY_DIR}/Imaging
${VTK_BINARY_DIR}/Graphics
${VTK_BINARY_DIR}/IO
)

IF (UNIX)
  LINK_LIBRARIES(${CMAKE_THREAD_LIBS} ${CMAKE_DL_LIBS} -lm)
ENDIF (UNIX)


OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library", OFF)

#
# Look for the PNG and zlib libraries and header files
#
# on windows we have it checked in

IF (WIN32)
  FIND_LIBRARY(PNG_LIBRARY libpng "${VTK_SOURCE_DIR}/IO/PNG")
  FIND_PATH(PNG_INCLUDE_PATH png.h "${VTK_SOURCE_DIR}/IO/PNG")
  FIND_PATH(ZLIB_INCLUDE_PATH zlib.h  "${VTK_SOURCE_DIR}/IO/PNG")
  FIND_LIBRARY(ZLIB_LIBRARY zlib  "${VTK_SOURCE_DIR}/IO/PNG")
ELSE (WIN32)
  FIND_LIBRARY(PNG_LIBRARY
    NAMES png libpng
    PATHS  
    /usr/lib 
    /usr/local/lib
  )
  FIND_PATH(PNG_INCLUDE_PATH png.h 
    /usr/include 
    /usr/local/include
    "${VTK_SOURCE_DIR}/IO/PNG"
  )
  FIND_LIBRARY(ZLIB_LIBRARY
    NAMES z zlib
    PATHS  
    /usr/lib 
    /usr/local/lib
  )
  FIND_PATH(ZLIB_INCLUDE_PATH zlib.h 
    /usr/include 
    /usr/local/include
    "${VTK_SOURCE_DIR}/IO/PNG"
  )
ENDIF (WIN32)

IF (PNG_LIBRARY)
  IF (PNG_INCLUDE_PATH)
    IF (ZLIB_INCLUDE_PATH)
      SET (HAVE_PNG 1)
      INCLUDE_DIRECTORIES(${PNG_INCLUDE_PATH})
    ENDIF (ZLIB_INCLUDE_PATH)
  ENDIF (PNG_INCLUDE_PATH)
ENDIF (PNG_LIBRARY)

#
# add include path for general testing header file
#
IF (BUILD_TESTING)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Common/Testing/Cxx)
ENDIF (BUILD_TESTING)

# where will executable tests be written ?
IF (EXECUTABLE_OUTPUT_PATH)
    SET (CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})
ELSE (EXECUTABLE_OUTPUT_PATH)
    SET (CXX_TEST_PATH .)
ENDIF (EXECUTABLE_OUTPUT_PATH)


#
# try to find the data files for VTKData
#
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${VTK_SOURCE_DIR}/../VTKData)
