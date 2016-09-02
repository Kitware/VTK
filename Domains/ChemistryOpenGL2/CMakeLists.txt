if(TARGET vtkRenderingOpenGL)
  message(FATAL_ERROR "vtkChemistyOpenGL2 cannot be built with vtkRenderingOpenGL, please disable one of them.")
endif()

include(vtkObjectFactory)

set(Module_SRCS
  vtkOpenGLMoleculeMapper.cxx
  )

# Now for the object factory.
set(opengl_overrides
  MoleculeMapper
  )

foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()
vtk_object_factory_configure("${vtk_module_overrides}")

list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(
  ${vtk-module}ObjectFactory.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkDomainsChemistryOpenGL2 ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(vtkDomainsChemistryOpenGL2)
