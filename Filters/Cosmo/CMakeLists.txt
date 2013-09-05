include(vtkMPI)
set(Module_SRCS
  vtkPCosmoReader.cxx
  vtkPCosmoHaloFinder.cxx)

vtk_module_library(${vtk-module} ${Module_SRCS})
vtk_mpi_link(${vtk-module})
