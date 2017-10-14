include(vtkObjectFactory)

mark_as_advanced(CLEAR Boost_INCLUDE_DIR)
find_package(Boost REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  mark_as_advanced(FORCE Boost_INCLUDE_DIR)
endif(Boost_FOUND)

set(Module_SRCS
  vtkPXdmf3Writer.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )
set_source_files_properties(
  ${vtk-module}ObjectFactory
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

# Now to generate our object factory.
vtk_add_override(vtkXdmf3Writer vtkPXdmf3Writer)
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(vtkIOParallelXdmf3 ${Module_SRCS})
