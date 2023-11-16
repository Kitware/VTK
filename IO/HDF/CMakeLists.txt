set(classes
  vtkHDFReader
  vtkHDFWriter)

set(private_classes
  vtkHDFReaderImplementation
  vtkHDFWriterImplementation)

set(headers
  vtkHDF5ScopedHandle.h
  vtkHDFUtilities.h)

set(private_headers
  vtkHDFVersion.h)

vtk_module_add_module(VTK::IOHDF
  CLASSES ${classes}
  NOWRAP_HEADERS ${headers}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers}
  )

vtk_add_test_mangling(VTK::IOHDF)
