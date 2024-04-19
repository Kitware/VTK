set(classes
  vtkVPICReader)

vtk_module_add_module(VTK::IOVPIC
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOVPIC)
