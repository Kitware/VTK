set(Module_SRCS
  SegyReader.cpp
  SegyReader.h
  BinaryHeaderBytesPositions.h
  TraceHeaderBytesPositions.h
  IOUtil.cpp
  IOUtil.h
  Trace.cpp
  Trace.h
  SegyTraceReader.cpp
  SegyTraceReader.h
  vtkSeismicSliceWidget.cpp
  vtkSeismicSliceWidget.h
  vtkSeismicSliceCallback.cpp
  vtkSeismicSliceCallback.h
  RdvReader.cpp
  RdvReader.h
  vtkSegy2DReader.cpp
  vtkSegy2DReader.h
  vtkSegy3DReader.cpp
  vtkSegy3DReader.h)

vtk_module_library(vtkIOSegY ${Module_SRCS})
