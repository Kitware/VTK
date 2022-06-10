set(classes
  vtkGeoProjection
  vtkGeoTransform)

vtk_module_add_module(VTK::GeovisCore
  CLASSES ${classes})
vtk_add_test_mangling(VTK::GeovisCore)
