vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOpenSlideReader.cxx
  "DATA{${VTK_TEST_INPUT_DIR}/Microscopy/Small.ndpi}"i
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
