set(Module_SRCS
  vtkDistributedDataFilter.cxx
  vtkPExtractGrid.cxx
  vtkPExtractRectilinearGrid.cxx
  vtkPExtractVOI.cxx
  vtkStructuredImplicitConnectivity.cxx
  )

vtk_module_library(vtkFiltersParallelMPI ${Module_SRCS})
