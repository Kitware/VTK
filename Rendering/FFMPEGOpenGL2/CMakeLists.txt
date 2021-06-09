set(classes
  vtkOpenGLMovieSphere
  )

vtk_module_add_module(VTK::RenderingFFMPEGOpenGL2
  CLASSES ${classes}
  )
