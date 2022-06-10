set(classes
  vtkHDFReader)

set(private_classes
  vtkHDFReaderImplementation)

vtk_module_add_module(VTK::IOHDF
  CLASSES ${classes}
  NOWRAP_HEADERS vtkHDF5ScopedHandle.h
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS vtkHDFReaderVersion.h
  )
vtk_add_test_mangling(VTK::IOHDF)
