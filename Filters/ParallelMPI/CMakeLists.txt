set(Module_SRCS
  vtkDistributedDataFilter.cxx
  )

vtk_module_library(vtkFiltersParallelMPI ${Module_SRCS})
