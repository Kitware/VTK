vtk_add_test_cxx(
  TestThreshold.cxx
  thresholdMain.cxx
  marchingMain.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
