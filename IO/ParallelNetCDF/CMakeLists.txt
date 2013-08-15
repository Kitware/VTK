include(vtkMPI)

set(Module_SRCS
  vtkPNetCDFPOPReader.cxx
  )

vtk_module_library(vtkIOParallelNetCDF ${Module_SRCS})

target_link_libraries(vtkIOParallelNetCDF LINK_PRIVATE vtkNetCDF_cxx)
vtk_mpi_link(vtkIOParallelNetCDF)
