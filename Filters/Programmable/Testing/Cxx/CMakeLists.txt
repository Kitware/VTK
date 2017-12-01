vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestProgrammableFilter.cxx,NO_VALID
  TestProgrammableGlyph.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY)
