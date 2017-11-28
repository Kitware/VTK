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

vtk_module_library(vtkDomainsChemistryOpenGL2 ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(vtkDomainsChemistryOpenGL2)
