include(vtkObjectFactory)

set(module_SRCS
  vtkOpenGLContextActor.cxx
  vtkOpenGLContextBufferId.cxx
  vtkOpenGLContextDevice2D.cxx
  vtkOpenGLContextDevice3D.cxx
  vtkOpenGLPropItem.cxx
  )

set(_opengl_overrides
  "ContextActor"
  "ContextDevice2D"
  "ContextDevice3D"
  "PropItem"
)
foreach(_override ${_opengl_overrides})
  vtk_add_override("vtk${_override}" "vtkOpenGL${_override}")
endforeach()
vtk_add_override("vtkAbstractContextBufferId" "vtkOpenGLContextBufferId")
vtk_object_factory_configure("${vtk_module_overrides}")

list(APPEND module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(${vtk-module} ${module_SRCS})
vtk_module_link_libraries(${vtk-module}
  LINK_PRIVATE ${GLEW_LIBRARY} ${OPENGL_LIBRARIES})
include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
