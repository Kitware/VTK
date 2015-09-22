include(vtkMPI)

# We don't build the parallel readers on windows
# because there's problems with the MPI_File_open()
# function when there's paths in the filenames.
set (vtk_module_overrides)
if(WIN32)
  set(Module_SRCS)
else()
  set(Module_SRCS
    vtkPWindBladeReader.cxx
    vtkMPIMultiBlockPLOT3DReader.cxx
    )

  vtk_add_override(vtkWindBladeReader vtkPWindBladeReader)
  vtk_add_override(vtkMultiBlockPLOT3DReader vtkMPIMultiBlockPLOT3DReader)
endif()

# Now to generate our object factory.
vtk_object_factory_configure("${vtk_module_overrides}")
list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )
set_source_files_properties(
  ${vtk-module}ObjectFactory
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )
vtk_module_library(${vtk-module} ${Module_SRCS})
vtk_mpi_link(${vtk-module})
