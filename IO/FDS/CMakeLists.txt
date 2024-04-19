set(classes
  vtkFDSReader)

vtk_module_add_module(VTK::IOFDS
  CLASSES ${classes}
  )

vtk_add_test_mangling(VTK::IOFDS)
