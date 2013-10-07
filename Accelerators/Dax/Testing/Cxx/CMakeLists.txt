vtk_add_test_cxx(
  TestDaxThreshold.cxx
  TestDaxThreshold2.cxx
  TestDaxMarchingCubes.cxx
  )


 vtk_test_cxx_executable(${vtk-module}CxxTests)
