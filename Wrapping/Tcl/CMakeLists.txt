# create the vtk executable
CONFIGURE_FILE(
  ${VTK_SOURCE_DIR}/Wrapping/Tcl/tkAppInit.cxx.in 
  ${VTK_BINARY_DIR}/Wrapping/Tcl/tkAppInit.cxx)

SET(LIBS
  vtkImagingTCL
  vtkGraphicsTCL
  vtkIOTCL
  vtkIO
  vtkGraphics
  vtkImaging
)

IF (TCL_LIBRARY)
  SET(LIBS ${LIBS} ${TCL_LIBRARY})
ELSE (TCL_LIBRARY)
  MESSAGE("VTK_WRAP_TCL is ON, but the Tcl library was not found.  Please set TCL_LIBRARY." "Error")
ENDIF (TCL_LIBRARY)


IF (VTK_USE_RENDERING)
  # add in the Tk values if found
#  IF (TK_INCLUDE_PATH)
#    INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
#    IF (APPLE)
#      INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Rendering/tkOSXInternals)
#    ENDIF (APPLE)
#  ENDIF (TK_INCLUDE_PATH)
#  IF (TK_XLIB_PATH)
#    INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
#  ENDIF (TK_XLIB_PATH)

  # add in xwindows stuff
#  IF (VTK_USE_X)
#    ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
#  ENDIF (VTK_USE_X)

  SET(LIBS ${LIBS} vtkRenderingTCL vtkRendering)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_PATENTED)
  SET(LIBS ${LIBS} vtkPatentedTCL vtkPatented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_HYBRID)
  SET(LIBS ${LIBS} vtkHybridTCL vtkHybrid)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PARALLEL)
  SET(LIBS ${LIBS} vtkParallelTCL vtkParallel)
ENDIF (VTK_USE_PARALLEL)

#
# link in the PNG library if it was found
#
ADD_EXECUTABLE(vtk ${VTK_BINARY_DIR}/Wrapping/Tcl/tkAppInit.cxx)

TARGET_LINK_LIBRARIES (vtk ${LIBS})

INSTALL_TARGETS(/bin vtk)

# Process all packages

SUBDIRS(vtk vtkbase vtkcommon vtkfiltering vtkgraphics vtkimaging vtkinteraction vtkio vtktesting)

INSTALL_FILES(/lib/vtk/tcl "" README)

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
