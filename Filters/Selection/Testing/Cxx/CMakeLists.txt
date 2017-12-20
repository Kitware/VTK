vtk_add_test_cxx(vtkFiltersSelectionCxxTests tests
  NO_VALID
  TestLinearSelector2D.cxx
  TestCellDistanceSelector2D.cxx
  TestCellDistanceSelector3D.cxx
  TestLinearSelector3D.cxx
  )
vtk_test_cxx_executable(vtkFiltersSelectionCxxTests tests
  RENDERING_FACTORY
  )
