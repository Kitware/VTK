set(classes
  vtkGeoJSONFeature
  vtkGeoJSONReader
  vtkGeoJSONWriter)

vtk_module_add_module(VTK::IOGeoJSON
  CLASSES ${classes})
