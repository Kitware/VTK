vtk_object_factory_declare(
  BASE vtkOpenGLGL2PSHelper
  OVERRIDE vtkOpenGLGL2PSHelperImpl)
vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGGL2PSOPENGL2_EXPORT")

set(classes
  vtkOpenGLGL2PSHelperImpl)

vtk_module_add_module(VTK::RenderingGL2PSOpenGL2
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
