PROJECT(VTK)
OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)

SUBDIRS(Wrapping Utilities Common Filtering Imaging Graphics IO)

SUBDIR_DEPENDS(Common Wrapping Utilities)
SUBDIR_DEPENDS(Filtering Common)
SUBDIR_DEPENDS(Imaging Filtering)
SUBDIR_DEPENDS(Graphics Filtering)
SUBDIR_DEPENDS(IO Filtering)

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
  SUBDIR_DEPENDS(Parallel Rendering)
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


