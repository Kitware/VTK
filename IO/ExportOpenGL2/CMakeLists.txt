include(vtkObjectFactory)
vtk_add_override(vtkGL2PSExporter vtkOpenGLGL2PSExporter)
vtk_object_factory_configure("${vtk_module_overrides}")

set(Module_SRCS
  vtkOpenGLGL2PSExporter.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
)

vtk_module_library(${vtk-module} ${Module_SRCS})

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
