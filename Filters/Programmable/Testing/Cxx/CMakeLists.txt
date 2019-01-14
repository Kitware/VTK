vtk_add_test_cxx(vtkFiltersProgrammableCxxTests tests
  TestProgrammableFilter.cxx,NO_VALID
  TestProgrammableGlyph.cxx
  )
vtk_test_cxx_executable(vtkFiltersProgrammableCxxTests tests
  RENDERING_FACTORY)
