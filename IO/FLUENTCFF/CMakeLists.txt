set(classes
  vtkFLUENTCFFReader)

set(private_classes
  vtkFLUENTCFFInternal
)

vtk_module_add_module(VTK::IOFLUENTCFF
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
)
  
vtk_add_test_mangling(VTK::IOFLUENTCFF)
