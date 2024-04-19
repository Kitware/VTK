set(classes
  vtkPythonArchiver)

vtk_module_add_module(VTK::CommonPython
  CLASSES ${classes})
vtk_add_test_mangling(VTK::CommonPython)
