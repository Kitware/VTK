set(classes
  vtkOpenGLContextActor
  vtkOpenGLContextBufferId
  vtkOpenGLContextDevice2D
  vtkOpenGLContextDevice3D
  vtkOpenGLPropItem)

set(private_headers
  vtkOpenGLContextDevice2DPrivate.h
  vtkOpenGLContextDeviceBufferObjectBuilder.h)

set(extra_sources
  vtkOpenGLContextDeviceBufferObjectBuilder.cxx)

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
  SOURCES ${vtk_object_factory_source} ${extra_sources}
  PRIVATE_HEADERS ${private_headers} ${vtk_object_factory_header})

vtk_add_test_mangling(VTK::RenderingContextOpenGL2)
