PROJECT(VTK)

BUILD_SHARED_LIBRARIES()

IF (WIN32)
  SET (LIBRARY_OUTPUT_PATH "../Lib/" CACHE STRING  
        "Directory for libraries to be built into.")
  SET (EXECUTABLE_OUTPUT_PATH "../Lib/" 
CACHE STRING 
        "Directory for executables to be built into.")
ELSE (WIN32)
  SET (LIBRARY_OUTPUT_PATH "" CACHE STRING 
        "Directory for libraries to be built into.")
  SET (EXECUTABLE_OUTPUT_PATH "" CACHE STRING 
        "Directory for executables to be built into.")
ENDIF (WIN32)

SUBDIRS(Wrapping Common Filtering Imaging Graphics IO)

INCLUDE (${VTK_SOURCE_DIR}/vtkCMakeOptions.cmake)

IF (VTK_USE_RENDERING)
  SUBDIRS(Rendering)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_HYBRID)
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
${VTK_SOURCE_DIR}/vtkToolkits.h.in 
${VTK_BINARY_DIR}/vtkToolkits.h)

INSTALL_FILES(/include/vtk .h vtkConfigure vtkToolkits)
