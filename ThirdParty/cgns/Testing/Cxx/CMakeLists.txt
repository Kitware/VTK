set(tests
  TestIncludeCGNS.cxx)

vtk_add_test_cxx(vtkcgnsCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkcgnsCxxTests tests)
