
# OpenSlideReader Tests

vtk_add_test_cxx(vtkDomainsMicroscopyCxxTests tests
  TestOpenSlideReader.cxx
  "DATA{${VTK_TEST_INPUT_DIR}/Microscopy/small2.ndpi}"
  )

vtk_add_test_cxx(vtkDomainsMicroscopyCxxTests  tests
    TestOpenSlideReaderPartialTIF,TestOpenSlideReaderPartial.cxx
  "DATA{${VTK_TEST_INPUT_DIR}/Microscopy/pathology.tif}"
  )

vtk_test_cxx_executable(vtkDomainsMicroscopyCxxTests tests RENDERING_FACTORY)
