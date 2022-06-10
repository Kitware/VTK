set(classes
  vtkTRUCHASReader)

vtk_module_add_module(VTK::IOTRUCHAS
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOTRUCHAS)
