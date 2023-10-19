set(classes
  vtkPIOReader)

set(private_classes
  BHTree
  PIOData
  PIODataPIO
  PIODataHDF5
  PIOAdaptor)

vtk_module_add_module(VTK::IOPIO
  CLASSES ${classes}
  EXCLUDE_HEADER_TEST
  PRIVATE_CLASSES ${private_classes})
vtk_add_test_mangling(VTK::IOPIO)
