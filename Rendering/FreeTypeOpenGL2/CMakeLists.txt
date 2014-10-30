include(vtkObjectFactory)

set(Module_SRCS
  vtkOpenGLFreeTypeTextMapper.cxx
  )

set(vtk_module_overrides "vtkTextMapper")
set(vtk_module_vtkTextMapper_override "vtkOpenGLFreeTypeTextMapper")
vtk_object_factory_configure("${vtk_module_overrides}")

list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(${vtk-module}ObjectFactory.cxx WRAP_EXCLUDE)

vtk_module_library(${vtk-module} ${Module_SRCS})
