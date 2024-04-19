set(classes
  vtkFLUENTCFFReader)

vtk_module_add_module(VTK::IOFLUENTCFF
  CLASSES ${classes}
  )
vtk_add_test_mangling(VTK::IOFLUENTCFF)
