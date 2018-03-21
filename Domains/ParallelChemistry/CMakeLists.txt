include(vtkMPI)
include(vtkObjectFactory)

set(Module_SRCS vtkPSimpleBondPerceiver.cxx)

vtk_add_override(vtkSimpleBondPerceiver vtkPSimpleBondPerceiver)
vtk_object_factory_configure("${vtk_module_overrides}")
list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(${vtk-module} ${Module_SRCS})
vtk_mpi_link(${vtk-module})
