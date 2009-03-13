SET(KIT Geovis)
SET(UKIT GEOVIS)
SET(KIT_TCL_LIBS vtkWidgetsTCL vtkViewsTCL)
SET(KIT_PYTHON_LIBS vtkWidgetsPythonD vtkViewsPythonD)
SET(KIT_JAVA_LIBS vtkWidgetsJava vtkViewsJava)
SET(KIT_LIBS vtkWidgets vtkViews ${VTK_LIBPROJ4_LIBRARIES})

SET( Kit_SRCS
  vtkCompassRepresentation.cxx
  vtkCompassWidget.cxx
  vtkGeoAdaptiveArcs.cxx
  vtkGeoAlignedImageRepresentation.cxx
  vtkGeoAlignedImageSource.cxx
  vtkGeoArcs.cxx
  vtkGeoAssignCoordinates.cxx
  vtkGeoCamera.cxx
  vtkGeoEdgeStrategy.cxx
  vtkGeoFileImageSource.cxx
  vtkGeoFileTerrainSource.cxx
  vtkGeoGlobeSource.cxx
  vtkGeoGraphRepresentation.cxx
  vtkGeoGraphRepresentation2D.cxx
  vtkGeoGraticule.cxx
  vtkGeoImageNode.cxx
  vtkGeoInteractorStyle.cxx
  vtkGeoLineRepresentation.cxx
  vtkGeoMath.cxx
  vtkGeoProjectionSource.cxx
  vtkGeoProjection.cxx
  vtkGeoRandomGraphSource.cxx
  vtkGeoSampleArcs.cxx
  vtkGeoSource.cxx
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
