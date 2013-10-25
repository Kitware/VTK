include(vtkMPI)

set(Module_SRCS
  vtkPNetCDFPOPReader.cxx
  )

vtk_module_library(vtkIOParallelNetCDF ${Module_SRCS})
vtk_mpi_link(vtkIOParallelNetCDF)
