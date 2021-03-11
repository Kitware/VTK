set(classes
  vtkCGNSFileSeriesReader
  vtkCGNSReader)

set(private_classes
  cgio_helpers
  vtkCGNSReaderInternal
  vtkFileSeriesHelper)

set(private_headers
  vtkCGNSCache.h)

vtk_module_add_module(VTK::IOCGNSReader
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers})
