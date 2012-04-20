find_package(MPI)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkPExodusIIReader.cxx
  )

vtk_module_library(vtkIOParallelExodus ${Module_SRCS})
