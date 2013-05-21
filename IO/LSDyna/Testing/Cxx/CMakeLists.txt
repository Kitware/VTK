vtk_add_test_cxx( LARGE_DATA
  TestLSDynaReader.cxx
  TestLSDynaReaderSPH.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
