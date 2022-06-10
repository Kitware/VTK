set(classes
  vtkOpenGLImageGradient
  )

vtk_module_add_module(VTK::ImagingOpenGL2
  CLASSES ${classes})
vtk_add_test_mangling(VTK::ImagingOpenGL2)
