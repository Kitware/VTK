PROJECT(VTK)
OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)

SUBDIRS(Wrapping Utilities Common Filtering Imaging Graphics IO)

SUBDIR_DEPENDS(Common Wrapping Utilities)
SUBDIR_DEPENDS(Filtering Common)
SUBDIR_DEPENDS(Imaging Filtering)
SUBDIR_DEPENDS(Graphics Filtering)
SUBDIR_DEPENDS(IO Filtering)

SET (LIBRARY_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${VTK_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all executables.")

OPTION (VTK_USE_RENDERING "Build the rendering classes used for displaying" ON)
OPTION (VTK_DEBUG_LEAKS "Build leak checking support into vtk." OFF)
OPTION (VTK_USE_HYBRID "Build the hybrid directory classes")
OPTION (VTK_USE_PATENTED "Build the patented directory classes, these classes are patented and may require a license to use")

OPTION (VTK_USE_PARALLEL "Build the parallel directory classes")
IF (VTK_USE_PARALLEL)
    OPTION (VTK_USE_MPI 
      "use MPI (Message Passing Interface) library for parallel support")
    IF(BUILD_TESTING)
     IF (DART_ROOT)
      IF (VTK_USE_MPI)
        FIND_PROGRAM(MPIRUN names mpirun lamexec)
        SET (MPI_PREFLAGS "-np 2" CACHE STRING " Flags used by mpi start program.")
        SET (MPI_POSTFLAGS "" CACHE STRING " Flags used by mpi start program.")
      ENDIF (VTK_USE_MPI)
     ENDIF (DART_ROOT)
    ENDIF(BUILD_TESTING)
ENDIF (VTK_USE_PARALLEL)

OPTION (VTK_MANGLE_MESA "Use mangled Mesa with OpenGL" OFF)
IF (VTK_MANGLE_MESA)
  FIND_LIBRARY(MESA_LIBRARY MesaGL
  /usr/local/lib 
  )
  FIND_LIBRARY(MESA_OS_LIBRARY MesaOS
  /usr/local/lib 
  )
ENDIF (VTK_MANGLE_MESA)

OPTION(VTK_WRAP_TCL "wrap classes into the TCL intepreted language")
IF (VTK_WRAP_TCL)
  INCLUDE (${CMAKE_ROOT}/Modules/FindTCL.cmake)
  UTILITY_SOURCE(VTK_WRAP_TCL_EXE vtkWrapTcl Wrapping vtkWrapTcl.c)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  IF (EXECUTABLE_OUTPUT_PATH)
    SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
  ELSE (EXECUTABLE_OUTPUT_PATH)
    SET (VTK_EXECUTABLE ${VTK_BINARY_DIR}/Wrapping/Tcl/vtk)
  ENDIF (EXECUTABLE_OUTPUT_PATH)
ENDIF (VTK_WRAP_TCL)

OPTION(VTK_WRAP_PYTHON "wrap classes into the Python interpreted language")
IF (VTK_WRAP_PYTHON)
  INCLUDE (${CMAKE_ROOT}/Modules/FindPythonLibs.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  UTILITY_SOURCE(VTK_WRAP_PYTHON_EXE vtkWrapPython Wrapping vtkWrapPython.c)
  IF(BUILD_TESTING)
   IF (DART_ROOT)
    FIND_PROGRAM(PYTHON_EXECUTABLE names python)
   ENDIF (DART_ROOT)
  ENDIF(BUILD_TESTING)
ENDIF (VTK_WRAP_PYTHON)

OPTION(VTK_WRAP_JAVA "wrap classes into the Java language")
IF (VTK_WRAP_JAVA)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJNI.cmake)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJNI.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  UTILITY_SOURCE(VTK_WRAP_JAVA_EXE vtkWrapJava Wrapping vtkWrapJava.c)
  UTILITY_SOURCE(VTK_PARSE_JAVA_EXE vtkParseJava Wrapping vtkParseJava.c)
  # where to write the resulting .java files
  IF (NOT VTK_JAVA_HOME)
    SET (VTK_JAVA_HOME . CACHE PATH "Path to Java install")
  ENDIF (NOT VTK_JAVA_HOME)
ENDIF (VTK_WRAP_JAVA)

OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library", OFF)

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

INCLUDE (${VTK_SOURCE_DIR}/vtkCMakeOptions.cmake)

IF (VTK_USE_RENDERING)
  SUBDIRS(Rendering)
  SUBDIR_DEPENDS(Rendering Imaging Graphics IO)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_HYBRID)
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

SUBDIRS(Work)

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
${VTK_SOURCE_DIR}/vtkToolkits.h.in 
${VTK_BINARY_DIR}/vtkToolkits.h)

# Include the standard Dart testing module
INCLUDE (${CMAKE_ROOT}/Modules/Dart.cmake)

# Add the testing directories
IF(BUILD_TESTING)
  IF (DART_ROOT)
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
  ELSE (DART_ROOT)
    MESSAGE("BUILD_TESTING requires DART_ROOT to be set!" "Error")
  ENDIF (DART_ROOT)
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


INSTALL_FILES(/include/vtk .h vtkConfigure vtkToolkits)


