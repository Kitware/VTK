set(classes
  vtkCompassRepresentation
  vtkCompassWidget
  vtkGeoProjection
  vtkGeoTransform)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND classes
    vtkGeoAdaptiveArcs
    vtkGeoAlignedImageRepresentation
    vtkGeoAlignedImageSource
    vtkGeoArcs
    vtkGeoAssignCoordinates
    vtkGeoCamera
    vtkGeoFileImageSource
    vtkGeoFileTerrainSource
    vtkGeoGlobeSource
    vtkGeoGraticule
    vtkGeoImageNode
    vtkGeoInteractorStyle
    vtkGeoProjectionSource
    vtkGeoRandomGraphSource
    vtkGeoSampleArcs
    vtkGeoSource
    vtkGeoSphereTransform
    vtkGeoTerrain
    vtkGeoTerrain2D
    vtkGeoTerrainNode
    vtkGeoTreeNode
    vtkGeoTreeNodeCache
    vtkGlobeSource)
endif ()

vtk_module_add_module(VTK::GeovisCore
  CLASSES ${classes})
