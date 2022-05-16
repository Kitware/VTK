set(classes
  vtkMappedUnstructuredGridGenerator
  vtkTestHTGGenerator)

vtk_module_add_module(VTK::TestingDataModel
  CLASSES ${classes})
