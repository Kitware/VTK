vtk_add_test_cxx(vtkFiltersMeshingCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestGeneralizedSurfaceNets3D.cxx
  TestShellBinIterator.cxx,NO_VALID
  TestVoronoiCore2D.cxx,NO_VALID
  TestVoronoiCore3D.cxx,NO_VALID
  )

set(all_tests
  ${tests}
  )

vtk_test_cxx_executable(vtkFiltersMeshingCxxTests all_tests)
