set(classes
  vtkPythonItem)

vtk_module_add_module(VTK::PythonContext2D
  CLASSES ${classes})
vtk_add_test_mangling(VTK::PythonContext2D)
