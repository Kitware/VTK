find_package(MPI)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkPNetCDFPOPReader.cxx
  )

vtk_module_library(vtkIOParallelNetCDF ${Module_SRCS})

target_link_libraries(vtkIOParallelNetCDF vtkNetCDF_cxx)
