set(classes
  H5RageAdaptor
  vtkH5RageReader)

vtk_module_add_module(VTK::IOH5Rage
  CLASSES ${classes})
