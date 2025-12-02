vtk_add_test_cxx(vtkFiltersMeshingCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestShellBinIterator.cxx,NO_VALID
  )

set(all_tests
  ${tests}
  )

vtk_test_cxx_executable(vtkFiltersMeshingCxxTests all_tests)
