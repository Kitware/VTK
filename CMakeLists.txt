PROJECT(VTK)

BUILD_SHARED_LIBRARIES()

SUBDIRS(Wrapping Common Filtering Imaging Graphics IO)

OPTION (VTK_USE_RENDERING "Build the rendering classes used for displaying")
IF (VTK_USE_RENDERING)
  INCLUDE (${CMAKE_SOURCE_DIR}/CMake/Modules/FindOpenGL.cmake)
  SUBDIRS(Rendering)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Rendering)
  LINK_DIRECTORIES(${VTK_BINARY_DIR}/Rendering) 
ENDIF (VTK_USE_RENDERING)

OPTION (VTK_USE_HYBRID "Build the hybrid directory classes")
IF (VTK_USE_HYBRID)
  # hybrid requires rendering
  IF (VTK_USE_RENDERING)
    SUBDIRS(Hybrid)
    INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Hybrid)
    LINK_DIRECTORIES(${VTK_BINARY_DIR}/Hybrid) 
  ELSE (VTK_USE_RENDERING)
  # error message should go here
  ENDIF (VTK_USE_RENDERING)
ENDIF (VTK_USE_HYBRID)

OPTION (VTK_USE_PATENTED "Build the patented directory classes, these classes are patented and may require a license to use")
IF (VTK_USE_PATENTED)
  SUBDIRS(Patented)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Patented)
  LINK_DIRECTORIES(${VTK_BINARY_DIR}/Patented) 
ENDIF (VTK_USE_PATENTED)

OPTION (VTK_USE_PARALLEL "Build the parallel directory classes")
IF (VTK_USE_PARALLEL)
    OPTION (VTK_USE_MPI 
      "use MPI (Message Passing Interface) library for parallel support")
    SUBDIRS(Parallel)
    INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Parallel)
    LINK_DIRECTORIES(${VTK_BINARY_DIR}/Parallel) 
ENDIF (VTK_USE_PARALLEL)

IF (WIN32)
  IF (BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DVTKDLL)
  ELSE (BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DVTKSTATIC)
  ENDIF (BUILD_SHARED_LIBS)
ENDIF (WIN32)

#
# get information for Tcl wrapping 
#
OPTION(VTK_WRAP_TCL "wrap classes into the TCL intepreted language")
IF (VTK_WRAP_TCL)
  INCLUDE (${CMAKE_SOURCE_DIR}/CMake/Modules/FindTCL.cmake)

  # add in the Tcl values if found
  IF (TCL_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
  ENDIF (TCL_INCLUDE_PATH)
  IF (TCL_LIB_PATH)
    LINK_DIRECTORIES (${TCL_LIB_PATH})
  ENDIF (TCL_LIB_PATH)
  IF (TCL_LIBRARY)
    LINK_LIBRARIES (${TCL_LIBRARY})
  ENDIF (TCL_LIBRARY)

  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  UTILITY_SOURCE(VTK_WRAP_TCL_EXE vtkWrapTcl Wrapping vtkWrapTcl.c)
ENDIF (VTK_WRAP_TCL)

#
# get information for Python wrapping 
#
OPTION(VTK_WRAP_PYTHON "wrap classes into the Python interpreted language")
IF (VTK_WRAP_PYTHON)
  FIND_LIBRARY(PY_LIB_PATH python21_d )
  FIND_LIBRARY(PY_LIB_PATH2 python21)
  FIND_PATH(PY_INCLUDE_PATH Python.h)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  UTILITY_SOURCE(VTK_WRAP_PYTHON_EXE vtkWrapPython Wrapping vtkWrapPython.c)
  INCLUDE_DIRECTORIES(${PY_INCLUDE_PATH})
  LINK_DIRECTORIES( ${PY_LIB_PATH} ${PY_LIB_PATH2} )
ENDIF (VTK_WRAP_PYTHON)

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
  LINK_LIBRARIES(${THREAD_LIBS} ${DL_LIBS} -lm)
ENDIF (UNIX)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/vtkConfigure.h.in 
${VTK_BINARY_DIR}/vtkConfigure.h)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/vtkToolkits.h.in 
${VTK_BINARY_DIR}/vtkToolkits.h)
