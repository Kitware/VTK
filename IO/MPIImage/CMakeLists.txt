include(vtkMPI)
include(vtkObjectFactory)

set(Module_SRCS
  vtkPNrrdReader.cxx
  vtkMPIImageReader.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )
set_source_files_properties(${vtk-module}ObjectFactory WRAP_EXCLUDE)

# Now to generate our object factory.
vtk_add_override(vtkNrrdReader vtkPNrrdReader)
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(vtkIOMPIImage ${Module_SRCS})
vtk_mpi_link(${vtk-module})
