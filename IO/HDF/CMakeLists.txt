set(classes
  vtkHDFReader)

set(private_classes
  vtkHDFReaderImplementation)

vtk_module_add_module(VTK::IOHDF
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS vtkHDFReaderVersion.h
  )
