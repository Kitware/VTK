set(classes
  vtkVeraOutReader)

vtk_module_add_module(VTK::IOVeraOut
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOVeraOut)
