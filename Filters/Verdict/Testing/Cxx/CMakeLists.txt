vtk_add_test_cxx(vtkFiltersVerdictCxxTests tests
  NO_VALID
  CellSizeFilter.cxx
  MeshQuality.cxx
  )
vtk_test_cxx_executable(vtkFiltersVerdictCxxTests tests)
