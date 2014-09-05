# Tests with data
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestVolumeAdditive.cxx
  TestVolumeClip.cxx
  TestVolumeCrop.cxx
  TestVolumeMinIP.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY)
