SET(KIT Geovis)
SET(UKIT GEOVIS)
SET(KIT_TCL_LIBS vtkWidgetsTCL vtkViewsTCL)
SET(KIT_PYTHON_LIBS vtkWidgetsPythonD vtkViewsPythonD)
SET(KIT_JAVA_LIBS vtkWidgetsJava vtkViewsJava)
SET(KIT_LIBS vtkWidgets vtkViews ${VTK_LIBXML2_LIBRARIES})

SET( Kit_SRCS
  vtkCompassRepresentation.cxx
  vtkCompassWidget.cxx
  vtkGeoAdaptiveArcs.cxx
  vtkGeoAlignedImage.cxx
  vtkGeoAlignedImageCache.cxx
  vtkGeoAlignedImageRepresentation.cxx
  vtkGeoAlignedImageSource.cxx
  vtkGeoArcs.cxx
  vtkGeoAssignCoordinates.cxx
  vtkGeoCamera.cxx
  vtkGeoComputeTextureCoordinates.cxx  
  vtkGeoEdgeStrategy.cxx
  vtkGeoGraphRepresentation.cxx
  vtkGeoImageNode.cxx
  vtkGeoInteractorStyle.cxx
  vtkGeoLineRepresentation.cxx
  vtkGeoMath.cxx
  vtkGeoPatch.cxx
  vtkGeoSampleArcs.cxx  
  vtkGeoTreeNode.cxx
  vtkGeoTerrain.cxx
  vtkGeoTerrainGlobeSource.cxx
  vtkGeoTerrainNode.cxx
  vtkGeoTerrainSource.cxx
  vtkGeoView.cxx
  vtkGlobeSource.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkGeoPatch.cxx
  ABSTRACT
)

SET_SOURCE_FILES_PROPERTIES(
  vtkGeoPatch.cxx
  WRAP_EXCLUDE
)

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
