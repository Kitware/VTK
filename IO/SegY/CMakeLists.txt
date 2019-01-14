set(classes
  vtkSegYReader)

set(sources
  vtkSegYIOUtils.cxx
  vtkSegYReaderInternal.cxx
  vtkSegYTraceReader.cxx)

set(private_headers
  vtkSegYIOUtils.h
  vtkSegYReaderInternal.h
  vtkSegYTraceReader.h)

vtk_module_add_module(VTK::IOSegY
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
