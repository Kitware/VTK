include(vtkObjectFactory)
vtk_add_override(vtkOpenGLGL2PSHelper vtkOpenGLGL2PSHelperImpl)
vtk_object_factory_configure("${vtk_module_overrides}")

set(Module_SRCS
  vtkOpenGLGL2PSHelperImpl.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
)

set_source_files_properties(
  vtkOpenGLGL2PSHelperImpl
  WRAP_EXCLUDE
)

vtk_module_library(${vtk-module} ${Module_SRCS})

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
