set(classes
  vtkCONVERGECFDReader)

vtk_module_add_module(VTK::IOCONVERGECFD
  CLASSES ${classes}
  SOURCES ${sources})
