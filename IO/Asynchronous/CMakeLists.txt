set(classes
  vtkThreadedImageWriter)

vtk_module_add_module(VTK::IOAsynchronous
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOAsynchronous)
