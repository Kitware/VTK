set(tests
  TestIncludeLibProj.cxx)

vtk_add_test_cxx(vtklibprojCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtklibprojCxxTests tests)
