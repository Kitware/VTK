set(classes
  vtkCompassRepresentation
  vtkCompassWidget
  vtkGeoProjection
  vtkGeoTransform)

vtk_module_add_module(VTK::GeovisCore
  CLASSES ${classes})
