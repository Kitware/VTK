set(classes
  vtkThreadedImageWriter)

vtk_module_add_module(VTK::IOAsynchronous
  CLASSES ${classes})
