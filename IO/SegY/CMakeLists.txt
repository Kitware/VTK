set(classes
  vtkSegYReader)

set(private_classes
  vtkSegYIOUtils
  vtkSegYReaderInternal
  vtkSegYTraceReader)

vtk_module_add_module(VTK::IOSegY
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes})
vtk_add_test_mangling(VTK::IOSegY)
