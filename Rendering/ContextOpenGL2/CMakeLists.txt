find_package(OpenGL REQUIRED)
include_directories(SYSTEM
  ${OPENGL_INCLUDE_DIR})

include(vtkObjectFactory)

set(module_SRCS
  vtkOpenGLContextActor.cxx
  vtkOpenGL2ContextDevice2D.cxx
  vtkOpenGLContextBufferId.cxx
  vtkOpenGLContextDevice2D.cxx
  vtkOpenGLContextDevice3D.cxx
  )

set_source_files_properties(
  vtkOpenGL2ContextDevice2D
  vtkOpenGLContextBufferId
  vtkOpenGLContextDevice2D
  vtkOpenGLContextDevice3D
  WRAP_EXCLUDE
  )

set(_opengl_overrides
  "ContextActor"
  "ContextDevice2D"
  "ContextDevice3D")
foreach(_override ${_opengl_overrides})
  vtk_add_override("vtk${_override}" "vtkOpenGL${_override}")
endforeach()
vtk_add_override("vtkAbstractContextBufferId" "vtkOpenGLContextBufferId")
vtk_object_factory_configure("${vtk_module_overrides}")

list(APPEND module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(${vtk-module}ObjectFactory WRAP_EXCLUDE)

vtk_module_library(${vtk-module} ${module_SRCS})
vtk_module_link_libraries(${vtk-module}
  LINK_PRIVATE ${GLEW_LIBRARY} ${OPENGL_LIBRARIES})
