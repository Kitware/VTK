set(classes
  vtkCGNSFileSeriesReader
  vtkCGNSReader)

set(sources
  cgio_helpers.cxx
  vtkCGNSReaderInternal.cxx
  vtkFileSeriesHelper.cxx)

set(private_headers
  cgio_helpers.h
  vtkCGNSCache.h
  vtkCGNSReaderInternal.h
  vtkFileSeriesHelper.h)

vtk_module_add_module(VTK::IOCGNSReader
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
