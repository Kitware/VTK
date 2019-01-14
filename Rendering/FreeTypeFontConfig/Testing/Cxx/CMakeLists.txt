vtk_add_test_cxx(vtkRenderingFreeTypeFontConfigCxxTests tests
  TestSystemFontRendering.cxx
  )
vtk_test_cxx_executable(vtkRenderingFreeTypeFontConfigCxxTests tests
    RENDERING_FACTORY)
