set(Module_SRCS
  vtkCompassRepresentation.cxx
  vtkCompassWidget.cxx
  vtkGeoAdaptiveArcs.cxx
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
  vtkGeoProjection.cxx
  vtkGeoProjectionSource.cxx
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
  vtkGlobeSource.cxx
  vtkGeoAlignedImageRepresentation.cxx
  )

set_source_files_properties(
  vtkGeoSource
  ABSTRACT
  )

vtk_module_library(vtkGeovisCore ${Module_SRCS})
