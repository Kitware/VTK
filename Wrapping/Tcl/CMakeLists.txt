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
TARGET_LINK_LIBRARIES (vtk
  vtkCommonTCL
  vtkFilteringTCL
  vtkGraphicsTCL
  vtkImagingTCL
  vtkIOTCL
)
IF(VTK_USE_CARBON)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
  IF(VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(
      SOURCE vtk
      COMMAND ${VTK_APPLE_RESOURCE}
      ARGS Carbon.r -o ${VTK_EXECUTABLE_DIR}/vtk
      TARGET vtk
      )
  ENDIF(VTK_APPLE_RESOURCE)
ENDIF(VTK_USE_CARBON)

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
    IF(VTK_USE_CARBON)
      FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
      IF(VTK_APPLE_RESOURCE)
        ADD_CUSTOM_COMMAND(
          SOURCE pvtk
          COMMAND ${VTK_APPLE_RESOURCE}
          ARGS Carbon.r -o ${VTK_EXECUTABLE_DIR}/pvtk
          TARGET pvtk
          )
      ENDIF(VTK_APPLE_RESOURCE)
    ENDIF(VTK_USE_CARBON)

    TARGET_LINK_LIBRARIES (pvtk
      vtkCommonTCL
      vtkFilteringTCL
      vtkGraphicsTCL
      vtkImagingTCL
      vtkIOTCL
      vtkParallelTCL
    )

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

SET(VTK_TCL_RENDERING_CONFIG "")
SET(VTK_TCL_HYBRID_CONFIG "")
SET(VTK_TCL_PATENTED_CONFIG "")
SET(VTK_TCL_PARALLEL_CONFIG "")

IF (VTK_USE_HYBRID)
  SET(VTK_TCL_HYBRID_CONFIG "Hybrid")
  SUBDIRS(vtkhybrid)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PARALLEL)
  SET(VTK_TCL_PARALLEL_CONFIG "Parallel")
  SUBDIRS(vtkparallel)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_USE_PATENTED)
  SET(VTK_TCL_PATENTED_CONFIG "Patented")
  SUBDIRS(vtkpatented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_RENDERING)
  SET(VTK_TCL_RENDERING_CONFIG "Rendering")
  SUBDIRS(vtkrendering)
ENDIF (VTK_USE_RENDERING)

# Configure the Tcl package index files for the build tree.
SET(VTK_TCL_SCRIPT_DIR "${VTK_SOURCE_DIR}/Wrapping/Tcl")

SET(VTK_TCL_LIBNAME_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")
SET(VTK_TCL_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})

IF(VTK_TCL_CONFIGURATION_TYPES)
  FOREACH(config ${VTK_TCL_CONFIGURATION_TYPES})
    SET(VTK_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}/${config}")
    CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                   ${VTK_BINARY_DIR}/Wrapping/Tcl/${config}/pkgIndex.tcl
                   @ONLY IMMEDIATE)
  ENDFOREACH(config)
ELSE(VTK_TCL_CONFIGURATION_TYPES)
  SET(VTK_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}")
  CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                 ${VTK_BINARY_DIR}/Wrapping/Tcl/pkgIndex.tcl
                 @ONLY IMMEDIATE)
ENDIF(VTK_TCL_CONFIGURATION_TYPES)

# Configure the Tcl package index file for the install tree.
SET(VTK_TCL_SCRIPT_DIR "[file dirname [info script]]")
IF(UNIX)
  SET(VTK_TCL_LIBRARY_DIR "[file dirname [file dirname [info script]]]")
ELSE(UNIX)
  SET(VTK_TCL_LIBRARY_DIR
      "[file join [file dirname [file dirname [file dirname [file dirname [info script]]]]] bin]")
ENDIF(UNIX)
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
               ${VTK_BINARY_DIR}/Wrapping/Tcl/Install/Hide/pkgIndex.tcl
               @ONLY IMMEDIATE)
INSTALL_FILES(/lib/vtk/tcl FILES
              ${VTK_BINARY_DIR}/Wrapping/Tcl/Install/Hide/pkgIndex.tcl)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
