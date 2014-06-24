find_package(OpenGL REQUIRED)
include_directories(SYSTEM
  ${OPENGL_INCLUDE_DIR})

set(module_SRCS
  vtkOpenGLContextActor.cxx
  vtkOpenGL2ContextDevice2D.cxx
  vtkOpenGLContextBufferId.cxx
  vtkOpenGLContextDevice2D.cxx
  vtkOpenGLContextDevice3D.cxx
  )

# Now we need to generate the object factory for this module. Create a list of
# overrides, and then generate the class that overrides those classes.
macro(vtk_add_override base override)
  list(APPEND vtk_module_overrides ${base})
  set(vtk_module_${base}_override ${override})
endmacro()
set(_opengl_overrides
  "ContextActor"
  "ContextDevice2D"
  "ContextDevice3D")
foreach(_override ${_opengl_overrides})
  vtk_add_override("vtk${_override}" "vtkOpenGL${_override}")
endforeach()
vtk_add_override("vtkAbstractContextBufferId" "vtkOpenGLContextBufferId")
# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()
# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
list(APPEND module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(${vtk-module} ${module_SRCS})
target_link_libraries(${vtk-module}
  LINK_PRIVATE ${OPENGL_LIBRARIES})
