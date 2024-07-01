set(classes
  vtkHDFReader
  vtkHDFWriter)

set(private_classes
  vtkHDFReaderImplementation
  vtkHDFWriterImplementation)

set(no_wrap_classes
  vtkHDFUtilities)

set(templates
  vtkHDFUtilities.txx)

set(private_headers
  vtkHDFVersion.h)

vtk_module_add_module(VTK::IOHDF
  CLASSES ${classes}
  TEMPLATES ${templates}
  NOWRAP_CLASSES ${no_wrap_classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers}
  )

vtk_add_test_mangling(VTK::IOHDF)
