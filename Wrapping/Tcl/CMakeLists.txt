CONFIGURE_FILE(
  ${VTK_SOURCE_DIR}/Wrapping/Tcl/vtkTkAppInitConfigure.h.in
  ${VTK_BINARY_DIR}/Wrapping/Tcl/vtkTkAppInitConfigure.h
  @ONLY IMMEDIATE
)

IF (NOT TCL_LIBRARY)
  MESSAGE("VTK_WRAP_TCL is ON, but the Tcl library was not found.  Please set TCL_LIBRARY." "Error")
ENDIF (NOT TCL_LIBRARY)

# Create the vtk Tcl wrapper executable.
ADD_EXECUTABLE(vtk vtkTkAppInit.cxx)
TARGET_LINK_LIBRARIES (vtk vtkGraphicsTCL vtkImagingTCL vtkIOTCL)

IF (VTK_USE_RENDERING)
  TARGET_LINK_LIBRARIES(vtk vtkRenderingTCL)
ENDIF (VTK_USE_RENDERING)
IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtk vtkPatentedTCL)
ENDIF (VTK_USE_PATENTED)
IF (VTK_USE_HYBRID)
  TARGET_LINK_LIBRARIES(vtk vtkHybridTCL)
ENDIF (VTK_USE_HYBRID)
IF (VTK_USE_PARALLEL)
  TARGET_LINK_LIBRARIES(vtk vtkParallelTCL)
ENDIF (VTK_USE_PARALLEL)

INSTALL_TARGETS(/bin vtk)

# Create the pvtk Tcl wrapper executable with MPI support.
IF (VTK_USE_PARALLEL)
  IF (VTK_USE_MPI)
    ADD_EXECUTABLE(pvtk vtkParaTkAppInit.cxx)
    TARGET_LINK_LIBRARIES (pvtk vtkParallelTCL)

    IF (VTK_USE_PATENTED)
      TARGET_LINK_LIBRARIES(pvtk vtkPatentedTCL)
    ENDIF (VTK_USE_PATENTED)
    IF (VTK_USE_HYBRID)
      TARGET_LINK_LIBRARIES(pvtk vtkHybridTCL)
    ENDIF (VTK_USE_HYBRID)

    INSTALL_TARGETS(/bin pvtk)
  ENDIF (VTK_USE_MPI)
ENDIF (VTK_USE_PARALLEL)

# Process all packages
SUBDIRS(vtk vtkbase vtkcommon vtkfiltering vtkgraphics vtkimaging vtkinteraction vtkio vtktesting)

INSTALL_FILES(/lib/vtk/tcl "" README)
INSTALL_FILES(/lib/vtk/tcl .c vtktcl)

IF (VTK_USE_HYBRID)
  SUBDIRS(vtkhybrid)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PARALLEL)
  SUBDIRS(vtkparallel)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_USE_PATENTED)
  SUBDIRS(vtkpatented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_RENDERING)
  SUBDIRS(vtkrendering)
ENDIF (VTK_USE_RENDERING)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
