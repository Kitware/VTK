SET(KIT Hybrid)
SET(UKIT HYBRID)

SET(KIT_TCL_LIBS vtkRenderingTCL vtkIOTCL)
IF(APPLE)
  SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkIOPythonD)
ELSE(APPLE)
  SET(KIT_PYTHON_LIBS vtkRenderingPython vtkIOPython)
ENDIF(APPLE)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkIOJava)
SET(KIT_LIBS vtkRendering vtkIO
  vtkexoIIc)

IF (WIN32)
  IF (VTK_USE_VIDEO_FOR_WINDOWS)
    SET(KIT_LIBS ${KIT_LIBS} vfw32)
  ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
  IF (VTK_USE_MATROX_IMAGING)
    SET(KIT_LIBS ${KIT_LIBS} ${MIL_LIBRARY})
  ENDIF (VTK_USE_MATROX_IMAGING)
ENDIF (WIN32)

SET ( Kit_SRCS
vtkPExodusReader.cxx
vtkExodusReader.cxx
vtkDSPFilterDefinition.cxx
vtkExodusModel.cxx
vtkDSPFilterGroup.cxx
vtk3DSImporter.cxx
vtkArcPlotter.cxx
vtkAnnotatedCubeActor.cxx
vtkAxesActor.cxx
vtkCaptionActor2D.cxx
vtkCornerAnnotation.cxx
vtkCubeAxesActor2D.cxx
vtkDepthSortPolyData.cxx
vtkEarthSource.cxx
vtkFacetReader.cxx
vtkGreedyTerrainDecimation.cxx
vtkGridTransform.cxx
vtkImageToPolyDataFilter.cxx
vtkImplicitModeller.cxx
vtkIterativeClosestPointTransform.cxx
vtkLandmarkTransform.cxx
vtkLegendBoxActor.cxx
vtkPCAAnalysisFilter.cxx
vtkPolyDataToImageStencil.cxx
vtkProcrustesAlignmentFilter.cxx
vtkProjectedTerrainPath.cxx
vtkRIBExporter.cxx
vtkRIBLight.cxx
vtkRIBProperty.cxx
vtkRenderLargeImage.cxx
vtkThinPlateSplineTransform.cxx
vtkTransformToGrid.cxx
vtkVRMLImporter.cxx
vtkVectorText.cxx
vtkVideoSource.cxx
vtkWeightedTransformFilter.cxx
vtkXYPlotActor.cxx
)

IF (WIN32)
  IF (VTK_USE_VIDEO_FOR_WINDOWS)
    SET (Kit_SRCS ${Kit_SRCS} vtkWin32VideoSource.cxx)
  ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)

  IF (VTK_USE_MATROX_IMAGING)
    SET (Kit_SRCS ${Kit_SRCS} vtkMILVideoSource.cxx)
    # Include directory needed for vtkMILVideoSource.cxx.
  ENDIF (VTK_USE_MATROX_IMAGING)
ENDIF (WIN32)

SET_SOURCE_FILES_PROPERTIES(
vtkVRML
WRAP_EXCLUDE)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

# -----------------------------------------------------------------
# Start of common section

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
                       "${Kit_SRCS}"
                       VTK_${UKIT}_EXPORT
                       ${VTK_BINARY_DIR} "")
ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})

# Allow the user to customize their build with some local options
#
SET(LOCALUSERMACRODEFINED 0)
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL3(vtk${KIT}TCL KitTCL_SRCS 
                "${Kit_SRCS}"
                "${Kit_TCL_EXTRA_CMDS}")
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS} ${Kit_TCL_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} ${KIT_TCL_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}TCL)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  IF(KIT_TCL_DEPS)
    ADD_DEPENDENCIES(vtk${KIT}TCL ${KIT_TCL_DEPS})
  ENDIF(KIT_TCL_DEPS)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON3(vtk${KIT}Python KitPython_SRCS "${Kit_SRCS}")
  IF (APPLE)
    ADD_LIBRARY(vtk${KIT}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    ADD_LIBRARY(vtk${KIT}Python MODULE vtk${KIT}PythonInit.cxx)
    TARGET_LINK_LIBRARIES(vtk${KIT}PythonD vtk${KIT} ${KIT_PYTHON_LIBS}
                          ${VTK_PYTHON_LIBRARIES})
    TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT}PythonD)
    IF(NOT VTK_INSTALL_NO_LIBRARIES)
      INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}PythonD)
    ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  ELSE (APPLE)
    ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtk${KIT} ${KIT_PYTHON_LIBS})
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python ${VTK_PYTHON_LIBRARIES})
  ENDIF(WIN32)
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Python)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  IF(KIT_PYTHON_DEPS)
    ADD_DEPENDENCIES(vtk${KIT}Python ${KIT_PYTHON_DEPS})
  ENDIF(KIT_PYTHON_DEPS)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA3(vtk${KIT}Java KitJava_SRCS "${Kit_SRCS}")
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(vtk${KIT}Java vtk${KIT} ${KIT_JAVA_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Java)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  IF(KIT_JAVA_DEPS)
    ADD_DEPENDENCIES(vtk${KIT}Java ${KIT_JAVA_DEPS})
  ENDIF(KIT_JAVA_DEPS)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_LIBS})

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT})
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h ${Kit_SRCS})
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

VTK_EXPORT_KIT("${KIT}" "${UKIT}" "${Kit_SRCS}")

# If the user defined a custom macro, execute it now and pass in all the srcs
#
IF(LOCALUSERMACRODEFINED)
  LocalUserOptionsMacro( "${Kit_SRCS}"       "${Kit_EXTRA_SRCS}"
                         "${KitTCL_SRCS}"    "${Kit_TCL_EXTRA_SRCS}"
                         "${KitJava_SRCS}"   "${Kit_JAVA_EXTRA_SRCS}"
                         "${KitPython_SRCS}" "${Kit_PYTHON_EXTRA_SRCS}")
ENDIF(LOCALUSERMACRODEFINED)

# End of common section
# -----------------------------------------------------------------
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h vtk3DS)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
