if (NOT WIN32)
  include(vtkMPI)
  include(vtkObjectFactory)

  set(Module_SRCS
    vtkPResampleToImage.cxx
    vtkPResampleWithDataSet.cxx
    )

  vtk_add_override(vtkResampleToImage vtkPResampleToImage)
  vtk_add_override(vtkResampleWithDataSet vtkPResampleWithDataSet)

  vtk_object_factory_configure("${vtk_module_overrides}")
  list(APPEND Module_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

  add_definitions(-DDIY_NO_THREADS)

  vtk_module_library(vtkFiltersParallelDIY2 ${Module_SRCS})
  vtk_mpi_link(vtkFiltersParallelDIY2)
endif()
