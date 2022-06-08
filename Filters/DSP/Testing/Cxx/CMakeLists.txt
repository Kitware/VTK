vtk_add_test_cxx(vtkFiltersDSPCxxTests tests
  TestBandFiltering.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkFiltersDSPCxxTests tests)
