vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestLinearSelector2D.cxx
  TestCellDistanceSelector2D.cxx
  TestCellDistanceSelector3D.cxx
  TestLinearSelector3D.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )
