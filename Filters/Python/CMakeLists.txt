set(classes
  vtkPythonAlgorithm)

vtk_module_add_module(VTK::FiltersPython
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersPython)
