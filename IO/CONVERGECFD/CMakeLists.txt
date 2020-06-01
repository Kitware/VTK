set(classes
  vtkCONVERGECFDReader)

vtk_module_add_module(VTK::IOCONVERGECFDReader
  CLASSES ${classes}
  SOURCES ${sources})
