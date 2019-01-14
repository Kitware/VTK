set(classes
  vtkOpenGLContextActor
  vtkOpenGLContextBufferId
  vtkOpenGLContextDevice2D
  vtkOpenGLContextDevice3D
  vtkOpenGLPropItem)

set(opengl_overrides
  ContextActor
  ContextDevice2D
  ContextDevice3D
  PropItem)
foreach (opengl_override IN LISTS opengl_overrides)
  vtk_object_factory_declare(
    BASE "vtk${opengl_override}"
    OVERRIDE "vtkOpenGL${opengl_override}")
endforeach ()
vtk_object_factory_declare(
  BASE vtkAbstractContextBufferId
  OVERRIDE vtkOpenGLContextBufferId)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGCONTEXTOPENGL2_EXPORT")

vtk_module_add_module(VTK::RenderingContextOpenGL2
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
