set(classes
  vtkMappedUnstructuredGridGenerator)

vtk_module_add_module(VTK::TestingDataModel
  CLASSES ${classes})
vtk_add_test_mangling(VTK::TestingDataModel)
