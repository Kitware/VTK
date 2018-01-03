set(classes
  ExternalVTKWidget
  vtkExternalLight
  vtkExternalOpenGLCamera
  vtkExternalOpenGLRenderWindow
  vtkExternalOpenGLRenderer)

vtk_module_add_module(VTK::RenderingExternal
  CLASSES ${classes})
vtk_module_definitions(VTK::RenderingExternal
  PRIVATE
    VTK_OPENGL2)
