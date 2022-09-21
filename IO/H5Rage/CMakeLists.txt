set(classes
  H5RageAdaptor
  vtkH5RageReader)

vtk_module_add_module(VTK::IOH5Rage
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOH5Rage)
