vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  CellSizeFilter.cxx
  MeshQuality.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
