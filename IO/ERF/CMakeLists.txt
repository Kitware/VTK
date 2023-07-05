set(classes
  vtkHDF5Helper
  vtkERFReader)

vtk_module_add_module(VTK::IOERF
  CLASSES ${classes}
  )
vtk_add_test_mangling(VTK::IOERF)
