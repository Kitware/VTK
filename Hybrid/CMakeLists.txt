SET(KIT Hybrid)
SET(UKIT HYBRID)
SET(KIT_TCL_LIBS vtkRenderingTCL vtkIOTCL )
IF(APPLE)
  SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkIOPythonD)
ELSE(APPLE)
  SET(KIT_PYTHON_LIBS vtkRenderingPython vtkIOPython)
ENDIF(APPLE)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkIOJava)
SET(KIT_LIBS vtkRendering vtkIO)

IF (WIN32)
  IF (VTK_USE_VIDEO_FOR_WINDOWS)
    SET(KIT_LIBS ${KIT_LIBS} vfw32)
  ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
  IF (VTK_USE_MATROX_IMAGING)
    SET(KIT_LIBS ${KIT_LIBS} ${MIL_LIBRARY})
  ENDIF (VTK_USE_MATROX_IMAGING)
ENDIF (WIN32)

SET ( Kit_SRCS
vtk3DSImporter.cxx
vtk3DWidget.cxx
vtkArcPlotter.cxx
vtkBoxWidget.cxx
vtkCaptionActor2D.cxx
vtkCubeAxesActor2D.cxx
vtkDepthSortPolyData.cxx
vtkEarthSource.cxx
vtkGreedyTerrainDecimation.cxx
vtkGridTransform.cxx
vtkImagePlaneWidget.cxx
vtkImageToPolyDataFilter.cxx
vtkImageTracerWidget.cxx
vtkImplicitModeller.cxx
vtkImplicitPlaneWidget.cxx
vtkIterativeClosestPointTransform.cxx
vtkLandmarkTransform.cxx
vtkLegendBoxActor.cxx
vtkLineWidget.cxx
vtkPCAAnalysisFilter.cxx
vtkPlaneWidget.cxx
vtkPointWidget.cxx
vtkPolyDataSourceWidget.cxx
vtkPolyDataToImageStencil.cxx
vtkProcrustesAlignmentFilter.cxx
vtkPushImageFilterSample.cxx
vtkPushImageReader.cxx
vtkPushPipeline.cxx
vtkRIBExporter.cxx
vtkRIBLight.cxx
vtkRIBProperty.cxx
vtkRenderLargeImage.cxx
vtkScalarBarWidget.cxx
vtkSphereWidget.cxx
vtkSplineWidget.cxx
vtkThinPlateSplineTransform.cxx
vtkTransformToGrid.cxx
vtkVRMLImporter.cxx
vtkVectorText.cxx
vtkVideoSource.cxx
vtkWeightedTransformFilter.cxx
vtkXYPlotActor.cxx
vtkXYPlotWidget.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtk3DWidget
vtkPolyDataSourceWidget
ABSTRACT
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

# -----------------------------------------------------------------
# Start of common section

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtk${KIT}Instantiator KitInstantiator_SRCS
                       ${Kit_SRCS}
                       EXPORT_MACRO VTK_${UKIT}_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtk${KIT} PROPERTIES LINK_FLAGS "-init _oft_init${KIT}")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
SET(LOCALUSERMACRODEFINED 0)
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtk${KIT}TCL SOURCES KitTCL_SRCS 
                ${Kit_SRCS}
                COMMANDS ${Kit_TCL_EXTRA_CMDS})
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS} ${Kit_TCL_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} ${KIT_TCL_LIBS})
  INSTALL_TARGETS(/lib/vtk vtk${KIT}TCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtk${KIT}Python KitPython_SRCS ${Kit_SRCS})
  IF (APPLE)
    ADD_LIBRARY(vtk${KIT}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    ADD_LIBRARY(vtk${KIT}Python MODULE vtk${KIT}PythonInit.cxx)
    TARGET_LINK_LIBRARIES(vtk${KIT}PythonD vtk${KIT} ${KIT_PYTHON_LIBS} ${PYTHON_LIBRARY})
    TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT}PythonD)
    INSTALL_TARGETS(/lib/vtk vtk${KIT}PythonD)
  ELSE (APPLE)
    ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtk${KIT} ${KIT_PYTHON_LIBS})
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Python)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtk${KIT}Java KitJava_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(vtk${KIT}Java vtk${KIT} ${KIT_JAVA_LIBS})
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Java)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_LIBS})

INSTALL_TARGETS(/lib/vtk vtk${KIT})
INSTALL_FILES(/include/vtk .h ${Kit_SRCS})

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
INSTALL_FILES(/include/vtk .h vtk3DS)
