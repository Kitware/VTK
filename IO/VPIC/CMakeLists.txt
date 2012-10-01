if(VTK_VPIC_USE_MPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  add_definitionS("-DMPICH_IGNORE_CXX_SEEK")
endif()

set(Module_SRCS
  vtkVPICReader.cxx
  )

vtk_module_library(vtkIOVPIC ${Module_SRCS})
