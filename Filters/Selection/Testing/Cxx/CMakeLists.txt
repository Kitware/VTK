vtk_add_test_cxx(NO_VALID
  TestLinearSelector2D.cxx
  )

# Tests with data
vtk_add_test_cxx(NO_VALID
  TestCellDistanceSelector2D.cxx
  TestCellDistanceSelector3D.cxx
  TestLinearSelector3D.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
