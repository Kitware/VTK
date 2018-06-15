set(Module_Deprecated_SRCS)

if(NOT VTK_LEGACY_REMOVE)
  list(APPEND Module_Deprecated_SRCS
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
    vtkGeoRandomGraphSource.cxx
    vtkGeoSampleArcs.cxx
    vtkGeoSource.cxx
    vtkGeoSphereTransform.cxx
    vtkGeoTerrain.cxx
    vtkGeoTerrain2D.cxx
    vtkGeoTerrainNode.cxx
    vtkGeoTreeNode.cxx
    vtkGeoTreeNodeCache.cxx
    vtkGlobeSource.cxx
    )
endif()

set(Module_SRCS
  ${Module_Deprecated_SRCS}
  vtkCompassRepresentation.cxx
  vtkCompassWidget.cxx
  vtkGeoProjection.cxx
  vtkGeoTransform.cxx
  )

vtk_module_library(vtkGeovisCore ${Module_SRCS})
