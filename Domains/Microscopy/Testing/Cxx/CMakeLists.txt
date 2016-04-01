
# OpenSlideReader Tests

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOpenSlideReader.cxx
  "DATA{${VTK_TEST_INPUT_DIR}/Microscopy/small2.ndpi}"
  )

vtk_add_test_cxx(${vtk-module}CxxTests  tests
    TestOpenSlideReaderPartialTIF,TestOpenSlideReaderPartial.cxx
  "DATA{${VTK_TEST_INPUT_DIR}/Microscopy/pathology.tif}"
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY)
