vtk_add_test_cxx(
  TestSMPContour.cxx,NO_VALID
  TestSMPTransform.cxx,NO_VALID
  TestSMPWarp.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
