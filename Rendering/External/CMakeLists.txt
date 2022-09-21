set(classes
  ExternalVTKWidget
  vtkExternalLight
  vtkExternalOpenGLCamera
  vtkExternalOpenGLRenderWindow
  vtkExternalOpenGLRenderer)

vtk_module_add_module(VTK::RenderingExternal
  CLASSES ${classes}
  EXCLUDE_HEADER_TEST)
vtk_module_definitions(VTK::RenderingExternal
  PRIVATE
    VTK_OPENGL2)
vtk_add_test_mangling(VTK::RenderingExternal)
