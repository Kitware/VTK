set(classes
  vtkOpenGLMovieSphere
  )

vtk_module_add_module(VTK::RenderingFFMPEGOpenGL2
  CLASSES ${classes}
  )
vtk_add_test_mangling(VTK::RenderingFFMPEGOpenGL2)
