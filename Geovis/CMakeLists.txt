SET(KIT Geovis)
SET(UKIT GEOVIS)
SET(KIT_TCL_LIBS vtkWidgetsTCL vtkViewsTCL)
SET(KIT_PYTHON_LIBS vtkWidgetsPythonD vtkViewsPythonD)
SET(KIT_JAVA_LIBS vtkWidgetsJava vtkViewsJava)
SET(KIT_INTERFACE_LIBRARIES vtkWidgets vtkViews)
SET(KIT_LIBS ${VTK_LIBPROJ4_LIBRARIES} ${OPENGL_LIBRARIES})

SET( Kit_SRCS
  vtkCompassRepresentation.cxx
  vtkCompassWidget.cxx
  vtkGeoAdaptiveArcs.cxx
  vtkGeoAlignedImageRepresentation.cxx
  vtkGeoAlignedImageSource.cxx
  vtkGeoArcs.cxx
  vtkGeoAssignCoordinates.cxx
  vtkGeoCamera.cxx
  vtkGeoFileImageSource.cxx
  vtkGeoFileTerrainSource.cxx
  vtkGeoGlobeSource.cxx
  vtkGeoGraticule.cxx
  vtkGeoImageNode.cxx
  vtkGeoInteractorStyle.cxx
  vtkGeoProjectionSource.cxx
  vtkGeoProjection.cxx
  vtkGeoRandomGraphSource.cxx
  vtkGeoSampleArcs.cxx
  vtkGeoSource.cxx
  vtkGeoSphereTransform.cxx
  vtkGeoTerrain.cxx
  vtkGeoTerrain2D.cxx
  vtkGeoTerrainNode.cxx
  vtkGeoTransform.cxx
  vtkGeoTreeNode.cxx
  vtkGeoTreeNodeCache.cxx
  vtkGeoView.cxx
  vtkGeoView2D.cxx
  vtkGlobeSource.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkGeoSource.cxx
  ABSTRACT
)

#SET_SOURCE_FILES_PROPERTIES(
#  WRAP_EXCLUDE
#)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
