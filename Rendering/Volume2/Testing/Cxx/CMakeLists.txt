# Tests without data
vtk_add_test_cxx(
  )

# Tests with data
vtk_add_test_cxx(
  TestGPURayCastCrop.cxx
  TestGPURayCastClip.cxx
  TestVolumeMinIP.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
