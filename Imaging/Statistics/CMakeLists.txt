set(Module_SRCS
  vtkImageAccumulate.cxx
  vtkImageHistogram.cxx
  vtkImageHistogramStatistics.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
