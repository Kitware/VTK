set(Module_SRCS
  vtkDistributedDataFilter.cxx
  vtkStructuredImplicitConnectivity.cxx
  )

vtk_module_library(vtkFiltersParallelMPI ${Module_SRCS})
