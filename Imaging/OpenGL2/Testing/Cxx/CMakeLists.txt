vtk_add_test_cxx(vtkImagingOpenGL2CxxTests tests
  TestOpenGLImageGradient.cxx
  )

vtk_test_cxx_executable(vtkImagingOpenGL2CxxTests tests RENDERING_FACTORY)
