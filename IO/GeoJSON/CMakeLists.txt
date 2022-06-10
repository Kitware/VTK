set(classes
  vtkGeoJSONFeature
  vtkGeoJSONReader
  vtkGeoJSONWriter)

vtk_module_add_module(VTK::IOGeoJSON
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOGeoJSON)
