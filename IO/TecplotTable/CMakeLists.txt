set(classes
  vtkTecplotTableReader)

vtk_module_add_module(VTK::IOTecplotTable
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOTecplotTable)
