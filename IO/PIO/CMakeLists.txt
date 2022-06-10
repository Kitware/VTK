set(classes
  BHTree
  PIOData
  PIOAdaptor
  vtkPIOReader)

vtk_module_add_module(VTK::IOPIO
  CLASSES ${classes}
  EXCLUDE_HEADER_TEST)
vtk_add_test_mangling(VTK::IOPIO)
