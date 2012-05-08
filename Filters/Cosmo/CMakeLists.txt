if(VTK_COSMOS_USE_MPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  add_definitionS("-DMPICH_IGNORE_CXX_SEEK")
else()
  add_definitions(-DUSE_SERIAL_COSMO)
endif()

set(Module_SRCS
  vtkPCosmoReader.cxx
  vtkPCosmoHaloFinder.cxx
  )

vtk_module_library(vtkFiltersCosmo ${Module_SRCS})
