find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkMPIImageReader.cxx
  vtkPNrrdReader.cxx
  )

vtk_module_library(vtkIOParallelMPI ${Module_SRCS})
