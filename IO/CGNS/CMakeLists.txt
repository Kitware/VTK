set(classes
  vtkCGNSFileSeriesReader
  vtkCGNSReader
  vtkCONVERGECFDCGNSReader)

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
vtk_add_test_mangling(VTK::IOCGNSReader)
