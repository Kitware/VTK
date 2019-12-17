set(classes
  BHTree
  PIOData
  PIOAdaptor
  vtkPIOReader)

vtk_module_add_module(VTK::IOPIO
  CLASSES ${classes})
