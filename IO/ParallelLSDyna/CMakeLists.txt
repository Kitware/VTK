set(classes
  vtkPLSDynaReader)

vtk_module_add_module(VTK::IOParallelLSDyna
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOParallelLSDyna)
