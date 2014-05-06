vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID NO_OUTPUT
  #    TestFlashReader.cxx  there is not data for this test
  TestEnzoReader.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
