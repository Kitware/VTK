set(classes
  vtkCityGMLReader)
vtk_module_add_module(VTK::IOCityGML
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOCityGML)
